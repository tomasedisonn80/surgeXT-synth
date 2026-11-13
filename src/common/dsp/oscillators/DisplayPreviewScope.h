/* Surge XT. GPL-3.0-or-later; see LICENSE. */
#ifndef SURGE_SRC_COMMON_DSP_OSCILLATORS_DISPLAYPREVIEWSCOPE_H
#define SURGE_SRC_COMMON_DSP_OSCILLATORS_DISPLAYPREVIEWSCOPE_H

namespace Surge::Oscillator
{
/** A narrow message-thread preview context. It never changes another thread's
    behavior or saves/reseeds a global RNG. Audible oscillator processing outside
    this scope retains its original drift and constructor-randomness behavior.
    A nested preview restores the caller's context on every return path. */
class DisplayPreviewScope final
{
  public:
    DisplayPreviewScope() noexcept : previous(activeFlag) { activeFlag = true; }
    ~DisplayPreviewScope() noexcept { activeFlag = previous; }
    static bool active() noexcept { return activeFlag; }

    DisplayPreviewScope(const DisplayPreviewScope &) = delete;
    DisplayPreviewScope &operator=(const DisplayPreviewScope &) = delete;

  private:
    inline static thread_local bool activeFlag{false};
    bool previous;
};
} // namespace Surge::Oscillator

#endif
