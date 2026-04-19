#pragma once

#include <algorithm>
#include <atomic>

#if defined(__x86_64__) || defined(_M_AMD64) || defined(__amd64__) || defined(__i386__) \
    || defined(_M_IX86)
#include <immintrin.h>
#define EXP_BACKOFF_X86_PAUSE 1
#endif

// Busy-wait helper for spin-lock loops: reduces pipeline churn on x86 (PAUSE).
inline void exp_backoff_pause_burst(int iterations)
{
    for (int i = 0; i < iterations; ++i) {
#if defined(EXP_BACKOFF_X86_PAUSE)
        _mm_pause();
#else
        std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
    }
}

// One exponential step: spin for `delay` pause-iters, then double `delay` (capped).
inline void exp_backoff_step(int& delay, int min_delay, int max_delay)
{
    exp_backoff_pause_burst(delay);
    delay = std::min(delay * 2, max_delay);
    if (delay < min_delay) {
        delay = min_delay;
    }
}

inline void exp_backoff_reset(int& delay, int min_delay)
{
    delay = min_delay;
}
