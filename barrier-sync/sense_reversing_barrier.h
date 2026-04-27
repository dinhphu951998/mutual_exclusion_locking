#pragma once

#include <atomic>

#include "../mutex-lock/exp_backoff.h"

class sense_reversing_barrier {
    const int N;                     // total number of threads (|T|)
    std::atomic<int>  count;         // already arrived threads
    std::atomic<bool> sense;         // global sense flag
    bool use_exponential_backoff;

    // Each thread has its own lsense
    static thread_local bool lsense;

public:
    static const unsigned kInitialDelayNs = 50;
    static const unsigned kMaxDelayNs = 6400;

    explicit sense_reversing_barrier(int num_threads, bool use_backoff = false)
        : N(num_threads), count(0), sense(true), use_exponential_backoff(use_backoff) {}

    void arrive_and_wait() {
        lsense = !lsense;

        int old = count.fetch_add(1, std::memory_order_acq_rel);
        if (old == N - 1) {
            // last thread to arrive
            count.store(0, std::memory_order_relaxed);    // prepare for next episode
            sense.store(lsense, std::memory_order_release);    // release threads
        } else {
            unsigned delay = kInitialDelayNs;
            while (sense.load(std::memory_order_acquire) != lsense) {
                if (use_exponential_backoff) {
                    exp_backoff_sleep_step(delay, kMaxDelayNs);
                }
            }
        }
    }
};

// Initialize lsense: true
inline thread_local bool sense_reversing_barrier::lsense = true;