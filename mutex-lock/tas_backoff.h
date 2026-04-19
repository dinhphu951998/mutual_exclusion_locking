#pragma once

#include <algorithm>
#include <atomic>

#include "exp_backoff.h"

// Test-and-set spin lock with exponential backoff between failed acquisitions.
class tas_spinlock_backoff {
    std::atomic_flag flag{};

public:
    static constexpr int kBackoffMin = 1;
    static constexpr int kBackoffMax = 4096;

    void lock()
    {
        int delay = kBackoffMin;
        while (flag.test_and_set(std::memory_order_acquire)) {
            exp_backoff_pause_burst(delay);
            delay = std::min(delay * 2, kBackoffMax);
        }
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};
