#pragma once

#include <algorithm>
#include <chrono>
#include <thread>

// One exponential step using sleep-based backoff.
inline void exp_backoff_sleep_step(unsigned& delay_ns, unsigned max_delay_ns)
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(delay_ns));
    delay_ns = std::min(delay_ns * 2U, max_delay_ns);
}
