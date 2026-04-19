#pragma once

#include <atomic>

#include "exp_backoff.h"

using namespace std;

class tas_spinlock
{
    atomic_flag flag{false};
    bool use_exponential_backoff;

    public:
        static const unsigned kInitialDelayNs = 50;
        static const unsigned kMaxDelayNs = 6400;

        explicit tas_spinlock(bool use_backoff = false) : use_exponential_backoff(use_backoff) {}

        void lock() {
            unsigned delay = kInitialDelayNs;
            while(flag.test_and_set(std::memory_order_acquire)) {
                if (use_exponential_backoff) {
                    exp_backoff_sleep_step(delay, kMaxDelayNs);
                }
            }
        }

        void unlock() {
            flag.clear(std::memory_order_release);
        }

};