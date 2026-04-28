#pragma once

#include <atomic>
#include <mutex>

#include "../mutex-lock/exp_backoff.h"

class MutexCounter {
public:
    static const unsigned kInitialDelayNs = 50;
    static const unsigned kMaxDelayNs = 6400;

    explicit MutexCounter(bool use_backoff = false)
        : use_exponential_backoff(use_backoff) {}

    void increment() {
        if (!use_exponential_backoff) {
            mtx.lock();
        } else {
            unsigned delay = kInitialDelayNs;
            while (!mtx.try_lock()) {
                exp_backoff_sleep_step(delay, kMaxDelayNs);
            }
        }
        ++value;
        mtx.unlock();
    }
    long long get() const { return value; }
private:
    long long value = 0;
    std::mutex mtx;
    bool use_exponential_backoff = false;
};

class CASCounter {
public:
    static const unsigned kInitialDelayNs = 50;
    static const unsigned kMaxDelayNs = 6400;

    explicit CASCounter(bool use_backoff = false)
        : use_exponential_backoff(use_backoff) {}

    void increment() {
        unsigned delay = kInitialDelayNs;
        long long old = value.load(std::memory_order_relaxed);
        while (!value.compare_exchange_weak(old, old + 1,
                                             std::memory_order_relaxed,
                                             std::memory_order_relaxed)) {
            if (use_exponential_backoff) {
                exp_backoff_sleep_step(delay, kMaxDelayNs);
            }
        }
    }
    long long get() const { return value.load(std::memory_order_relaxed); }
private:
    std::atomic<long long> value{0};
    bool use_exponential_backoff = false;
};

class FetchAddCounter {
public:
    
    explicit FetchAddCounter(bool use_backoff = false)
    {
        // Backoff is intentionally ignored for fetch_add since there is no retry loop.
        (void)use_backoff;
    }

    void increment() {
        value.fetch_add(1, std::memory_order_relaxed);
    }

    long long get() const {
        return value.load(std::memory_order_relaxed);
    }

private:
    std::atomic<long long> value{0};
};