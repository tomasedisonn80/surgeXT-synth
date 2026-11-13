// Standalone concurrency regression tests. GPL-3.0-or-later; see LICENSE.
// No JUCE/native GUI packages are needed to compile this production-header test.
#include "../../src/surge-xt/gui/reference/AudioTap.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>

namespace
{
thread_local bool audioMustNotAllocate = false;
}

void *operator new(std::size_t bytes)
{
    if (audioMustNotAllocate)
        std::abort();
    if (auto *memory = std::malloc(bytes == 0 ? 1 : bytes))
        return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

namespace
{
using Surge::GUI::Reference::AudioTap;

void exactBoundaries()
{
    AudioTap<8> tap;
    std::array<float, 10> left{}, right{};
    for (int i = 0; i < 10; ++i)
    {
        left[i] = static_cast<float>(i + 1);
        right[i] = -left[i];
    }
    assert(!tap.subscribed());
    assert(tap.push(left.data(), right.data(), 10) == 0);
    assert(tap.popall().first.empty());
    tap.subscribe();
    tap.subscribe(); // idempotent; does not discard a currently active queue
    assert(tap.subscribed());
    assert(tap.push(nullptr, right.data(), 10) == 0);
    assert(tap.push(left.data(), nullptr, 10) == 0);
    assert(tap.push(left.data(), right.data(), 0) == 0);
    assert(tap.push(left.data(), right.data(), 10) == 8);
    assert(tap.droppedFrames() == 2);
    assert(tap.push(left.data(), right.data(), 1) == 0);
    assert(tap.droppedFrames() == 3);
    const auto full = tap.popall();
    assert(full.first.size() == 8 && full.second.size() == 8);
    for (int i = 0; i < 8; ++i)
    {
        assert(full.first[i] == i + 3);
        assert(full.second[i] == -full.first[i]);
    }
    assert(tap.popall().first.empty());

    // Exercise a physical wrap, then verify a partial write retains its newest
    // input frames without changing six unread frames already in the queue.
    assert(tap.push(left.data(), right.data(), 6) == 6);
    assert(tap.popall().first.size() == 6);
    assert(tap.push(left.data(), right.data(), 6) == 6);
    assert(tap.push(left.data() + 7, right.data() + 7, 3) == 2);
    const auto wrap = tap.popall();
    assert(wrap.first.size() == 8);
    for (int i = 0; i < 6; ++i)
        assert(wrap.first[i] == i + 1);
    assert(wrap.first[6] == 9 && wrap.first[7] == 10);

    assert(tap.push(left.data(), right.data(), 5) == 5);
    tap.unsubscribe();
    tap.unsubscribe();
    assert(!tap.subscribed());
    assert(tap.push(left.data(), right.data(), 1) == 0);
    tap.subscribe();
    assert(tap.popall().first.empty()); // no stale pre-hide audio
    assert(tap.push(left.data() + 9, right.data() + 9, 1) == 1);
    assert(tap.popall().first.front() == 10);
}

void concurrentStress(bool changeSubscriptions)
{
    AudioTap<64> tap;
    tap.subscribe();
    constexpr std::uint32_t totalFrames = 4000000;
    std::atomic<bool> producerDone{false};
    std::atomic<bool> firstPush{false};
    std::uint64_t accepted{0}; // written by producer, read only after join
    std::thread producer([&] {
        std::array<float, 257> left{}, right{};
        std::uint32_t sequence{0}, random{0x12345678};
        while (sequence < totalFrames)
        {
            random = random * 1664525u + 1013904223u;
            const auto count = std::min(totalFrames - sequence, 1u + (random % 257));
            for (std::uint32_t i = 0; i < count; ++i)
            {
                left[i] = static_cast<float>(++sequence);
                right[i] = -left[i];
            }
            audioMustNotAllocate = true;
            accepted += tap.push(left.data(), right.data(), count);
            audioMustNotAllocate = false;
            firstPush.store(true, std::memory_order_release);
            if ((random & 63u) == 0)
                std::this_thread::yield();
        }
        producerDone.store(true, std::memory_order_release);
    });

    float previous{0};
    std::uint64_t consumed{0};
    std::uint32_t reads{0};
    while (!firstPush.load(std::memory_order_acquire))
        std::this_thread::yield();
    const auto consume = [&] {
        const auto data = tap.popall();
        assert(data.first.size() == data.second.size());
        assert(data.first.size() <= tap.capacity);
        for (std::size_t i = 0; i < data.first.size(); ++i)
        {
            // Detect torn stereo frames, overwrite-induced reordering, duplicate
            // reads, and values outside the exact-integer float range of the test.
            assert(data.first[i] == -data.second[i]);
            assert(data.first[i] > previous && data.first[i] <= totalFrames);
            previous = data.first[i];
            ++consumed;
        }
    };
    while (!producerDone.load(std::memory_order_acquire))
    {
        consume();
        if (changeSubscriptions && (++reads % 7) == 0)
        {
            tap.unsubscribe();
            std::this_thread::yield();
            tap.subscribe();
        }
        // Force the producer to outrun its reader and exercise the full queue.
        if ((reads++ & 31u) == 0)
            std::this_thread::yield();
    }
    producer.join();
    consume();
    assert(tap.popall().first.empty());
    assert(consumed > 0 && accepted >= consumed);
    if (!changeSubscriptions)
    {
        assert(consumed == accepted);
        assert(accepted + tap.droppedFrames() == totalFrames);
    }
    std::cout << (changeSubscriptions ? "subscription lifecycle" : "continuous subscription")
              << ": accepted=" << accepted << ", consumed=" << consumed
              << ", dropped=" << tap.droppedFrames() << '\n';
}
} // namespace

int main()
{
    exactBoundaries();
    concurrentStress(false);
    concurrentStress(true);
    std::cout << "PASS: AudioTap boundaries, wrap, stalls, stereo ordering, lifecycle, no audio "
                 "allocations\n";
}
