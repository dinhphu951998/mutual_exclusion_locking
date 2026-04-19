#pragma once

#include <atomic>

#include "exp_backoff.h"

using namespace std;

class ttas_spinlock
{
    atomic<bool> flag{false};
    bool use_exponential_backoff;

    public:
        static const unsigned kInitialDelayNs = 50;
        static const unsigned kMaxDelayNs = 6400;

        explicit ttas_spinlock(bool use_backoff = false) : use_exponential_backoff(use_backoff) {}

        void lock() {
            unsigned delay = kInitialDelayNs;
            do {
                while(flag.load(std::memory_order_relaxed)) {
                    if (use_exponential_backoff) {
                        exp_backoff_sleep_step(delay, kMaxDelayNs);
                    }
                }
                if (!flag.exchange(true, std::memory_order_acquire)) {
                    return;
                }
                if (use_exponential_backoff) {
                    exp_backoff_sleep_step(delay, kMaxDelayNs);
                }
            } while(true);
        }

        void unlock() {
            flag.store(false, std::memory_order_release);
        }

};