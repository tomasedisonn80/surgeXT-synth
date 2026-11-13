/* Surge XT reference workspaces. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_REFERENCE_AUDIO_TAP_H
#define SURGE_REFERENCE_AUDIO_TAP_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace Surge::GUI::Reference
{

/** Bounded single-producer/single-consumer display tap.

    Only the audio thread calls push(). Only the message thread calls subscribe(),
    unsubscribe(), and popall(). Destruction requires both threads to have stopped
    using the tap, as guaranteed by its processor lifetime. There is deliberately
    no clear/reset method that could modify the producer's index from the UI.

    The producer never overwrites unread storage: a stalled UI loses new display
    frames rather than introducing a data race or delaying audio. A partial fit
    retains the newest frames of that callback. Queue publication and reclamation
    use acquire/release ordering. All audio-side operations are bounded, lock-free,
    allocation-free, and do not alter the supplied audio buffers.

    Subscription generations discard any in-flight frames from before hide/reopen.
    popall() may allocate on the message thread; it never runs on the audio thread.
*/
template <std::size_t Capacity = 8192> class AudioTap
{
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                  "AudioTap capacity must be a power of two");
    static_assert(Capacity < (std::uint32_t{1} << 31), "AudioTap capacity is too large");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "AudioTap requires lock-free 32-bit atomics");

  public:
    static constexpr std::size_t capacity = Capacity;

    void subscribe() noexcept
    {
        const auto generation = subscription_.load(std::memory_order_relaxed);
        if ((generation & 1u) != 0)
            return;
        // Only the consumer reclaims slots. An old producer may still publish
        // after this load; its per-frame generation will be rejected by popall.
        read_.store(write_.load(std::memory_order_acquire), std::memory_order_release);
        subscription_.store(generation + 1u, std::memory_order_release);
    }

    void unsubscribe() noexcept
    {
        const auto generation = subscription_.load(std::memory_order_relaxed);
        if ((generation & 1u) != 0)
            subscription_.store(generation + 1u, std::memory_order_release);
    }

    bool subscribed() const noexcept
    {
        return (subscription_.load(std::memory_order_acquire) & 1u) != 0;
    }

    /** Returns accepted frames. Oversize/full-queue losses count toward the
        saturating display-only counter; inactive/invalid input is not counted. */
    std::size_t push(const float *left, const float *right, std::size_t frames) noexcept
    {
        const auto generation = subscription_.load(std::memory_order_acquire);
        if ((generation & 1u) == 0 || !left || !right || frames == 0)
            return 0;

        const auto write = write_.load(std::memory_order_relaxed);
        const auto read = read_.load(std::memory_order_acquire);
        const auto used = static_cast<std::uint32_t>(write - read);
        const auto count = std::min(frames, Capacity - static_cast<std::size_t>(used));
        const auto skipped = frames - count;
        if (skipped != 0)
        {
            const auto previous = dropped_.load(std::memory_order_relaxed);
            const auto room = std::numeric_limits<std::uint32_t>::max() - previous;
            dropped_.store(previous +
                               static_cast<std::uint32_t>(std::min<std::size_t>(room, skipped)),
                           std::memory_order_relaxed);
        }
        for (std::size_t i = 0; i < count; ++i)
        {
            auto &slot = frames_[(write + i) & (Capacity - 1)];
            slot.left = left[skipped + i];
            slot.right = right[skipped + i];
            slot.generation = generation;
        }
        if (count != 0)
            write_.store(write + static_cast<std::uint32_t>(count), std::memory_order_release);
        return count;
    }

    std::pair<std::vector<float>, std::vector<float>> popall()
    {
        const auto generation = subscription_.load(std::memory_order_relaxed);
        const auto read = read_.load(std::memory_order_relaxed);
        const auto write = write_.load(std::memory_order_acquire);
        const auto count = static_cast<std::uint32_t>(write - read);
        std::pair<std::vector<float>, std::vector<float>> result;
        if ((generation & 1u) != 0 && count != 0)
        {
            result.first.reserve(count);
            result.second.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i)
            {
                const auto &slot = frames_[(read + i) & (Capacity - 1)];
                if (slot.generation == generation)
                {
                    result.first.push_back(slot.left);
                    result.second.push_back(slot.right);
                }
            }
        }
        // Reclaim only after every float and generation has been copied.
        read_.store(write, std::memory_order_release);
        return result;
    }

    std::uint32_t droppedFrames() const noexcept
    {
        return dropped_.load(std::memory_order_relaxed);
    }

  private:
    struct Frame
    {
        float left{0}, right{0};
        std::uint32_t generation{0};
    };
    std::array<Frame, Capacity> frames_{};
    alignas(64) std::atomic<std::uint32_t> write_{0};
    alignas(64) std::atomic<std::uint32_t> read_{0};
    alignas(64) std::atomic<std::uint32_t> subscription_{0};
    std::atomic<std::uint32_t> dropped_{0};
};

} // namespace Surge::GUI::Reference
#endif
