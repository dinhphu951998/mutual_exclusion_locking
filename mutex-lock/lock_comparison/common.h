#pragma once

#include <cassert>
#include <chrono>
#include <cstddef>
#include <pthread.h>
#include <thread>
#include <vector>

#include <omp.h>

namespace lock_comparison {

inline int is_prime(int p)
{
    for (int d = 2; d < p; ++d) {
        if (p % d == 0) {
            return 0;
        }
    }
    return 1;
}

template <typename Worker>
inline double time_std_threads(int num_threads, Worker worker)
{
    std::vector<std::thread> threads(static_cast<size_t>(num_threads));

    const auto t0 = std::chrono::steady_clock::now();
    for (auto& th : threads) {
        th = std::thread(worker);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template <typename LockType>
inline void run_lockable_workload(LockType* lock, int num_iters, int outside_work, int n)
{
    for (int i = 0; i < num_iters; ++i) {
        lock->lock();
        assert(is_prime(n) == 1);
        lock->unlock();

        for (int j = 0; j < outside_work; ++j) {
            assert(is_prime(n) == 1);
        }
    }
}

template <typename LockType>
inline double time_std_threads_lock(LockType* lock, int num_threads, int num_iters, int outside_work, int n)
{
    return time_std_threads(
        num_threads,
        [lock, num_iters, outside_work, n]() {
            run_lockable_workload(lock, num_iters, outside_work, n);
        }
    );
}

class pthread_mutex_wrapper
{
public:
    pthread_mutex_wrapper()
    {
        pthread_mutex_init(&mutex_, nullptr);
    }

    ~pthread_mutex_wrapper()
    {
        pthread_mutex_destroy(&mutex_);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex_);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }

private:
    pthread_mutex_t mutex_{};
};

class omp_lock_wrapper
{
public:
    omp_lock_wrapper()
    {
        omp_init_lock(&lock_);
    }

    ~omp_lock_wrapper()
    {
        omp_destroy_lock(&lock_);
    }

    void lock()
    {
        omp_set_lock(&lock_);
    }

    void unlock()
    {
        omp_unset_lock(&lock_);
    }

private:
    omp_lock_t lock_{};
};

template <typename LockType>
struct pthread_worker_args {
    LockType* lock;
    int num_iters;
    int outside_work;
    int n;
};

template <typename LockType>
inline void* pthread_worker(void* raw_args)
{
    auto* args = static_cast<pthread_worker_args<LockType>*>(raw_args);
    run_lockable_workload(args->lock, args->num_iters, args->outside_work, args->n);
    return nullptr;
}

template <typename LockType>
inline double time_pthread_lock(LockType* lock, int num_threads, int num_iters, int outside_work, int n)
{
    std::vector<pthread_t> threads(static_cast<size_t>(num_threads));
    std::vector<pthread_worker_args<LockType>> args(
        static_cast<size_t>(num_threads),
        {lock, num_iters, outside_work, n}
    );

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(
            &threads[static_cast<size_t>(i)],
            nullptr,
            pthread_worker<LockType>,
            &args[static_cast<size_t>(i)]
        );
    }
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[static_cast<size_t>(i)], nullptr);
    }
    const auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template <typename LockType>
inline double time_openmp_lock(LockType* lock, int num_threads, int num_iters, int outside_work, int n)
{
    const auto t0 = std::chrono::steady_clock::now();
#pragma omp parallel num_threads(num_threads) default(none) shared(lock, num_iters, outside_work, n)
    {
        run_lockable_workload(lock, num_iters, outside_work, n);
    }
    const auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

}  // namespace lock_comparison
