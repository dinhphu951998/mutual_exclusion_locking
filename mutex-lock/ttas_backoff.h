#pragma once

#include <algorithm>
#include <atomic>

#include "exp_backoff.h"

// Test-and-test-and-set spin lock with exponential backoff while waiting.
class ttas_spinlock_backoff {
    std::atomic<bool> flag{false};

public:
    static constexpr int kBackoffMin = 1;
    static constexpr int kBackoffMax = 4096;

    void lock()
    {
        int delay = kBackoffMin;
        for (;;) {
            while (flag.load(std::memory_order_relaxed)) {
                exp_backoff_pause_burst(delay);
                delay = std::min(delay * 2, kBackoffMax);
            }
            if (!flag.exchange(true, std::memory_order_acquire)) {
                return;
            }
            exp_backoff_pause_burst(delay);
            delay = std::min(delay * 2, kBackoffMax);
        }
    }

    void unlock()
    {
        flag.store(false, std::memory_order_release);
    }
};
