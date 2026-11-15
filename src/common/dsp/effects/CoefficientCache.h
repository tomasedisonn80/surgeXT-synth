/*
 * Surge XT - a free and open source hybrid synthesizer,
 * released under the GNU General Public Licence v3 or later.
 */

#ifndef SURGE_SRC_COMMON_DSP_EFFECTS_COEFFICIENTCACHE_H
#define SURGE_SRC_COMMON_DSP_EFFECTS_COEFFICIENTCACHE_H

#include <cstdint>
#include "globals.h"

#if defined(SST_SIMD_ARM64) && defined(_MSC_VER)
#include <intrin.h>
#endif

namespace Surge::DSP
{
struct CoefficientCacheEnvironment
{
    std::uint64_t bits;
    bool supported;
};

inline CoefficientCacheEnvironment coefficientCacheEnvironment() noexcept
{
    // ARM64EC also exposes x86 compatibility macros, so select native ARM first.
#if defined(SST_SIMD_ARM64)
#if defined(_MSC_VER)
    // ARM64_SYSREG(3, 3, 4, 4, 0), the FPCR encoding accepted by _ReadStatusReg.
    // Use the encoding because older compiler headers omit ARM64_FPCR.
    constexpr int fpcrRegister = 0x5a20;
    return {static_cast<std::uint64_t>(_ReadStatusReg(fpcrRegister)), true};
#elif defined(__GNUC__) || defined(__clang__)
    std::uint64_t fpcr;
    asm volatile("MRS %0, FPCR" : "=r"(fpcr));
    // FPCR contains controls; cumulative exception status is in separate FPSR.
    return {fpcr, true};
#else
    return {0, false};
#endif
#elif defined(SST_SIMD_NATIVE_X86)
    // Include rounding, flush-to-zero, and denormals-are-zero, not sticky status.
    auto bits = static_cast<std::uint64_t>(_mm_getcsr() & 0xe040u);
#if defined(__GNUC__) || defined(__clang__)
    // The engine uses SSE arithmetic, but libm may also inspect/use x87 state.
    // Preserve its precision and rounding controls without including status.
    std::uint16_t x87Control;
    asm volatile("fnstcw %0" : "=m"(x87Control));
    bits |= static_cast<std::uint64_t>(x87Control & 0x0f00u) << 32;
#elif defined(_MSC_VER) && defined(_M_IX86)
    // Avoid a runtime control-word query in legacy 32-bit MSVC callbacks.
    return {0, false};
#endif
    return {bits, true};
#else
    // An incomplete architecture key could retain targets from another mode.
    // Preserve uncached behavior on platforms without a native control read.
    return {0, false};
#endif
}
} // namespace Surge::DSP

#endif
