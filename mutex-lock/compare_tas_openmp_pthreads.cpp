#include <cassert>
#include <chrono>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <vector>
#include <cstddef>

#include <omp.h>

#include "tas.h"
#include "ttas.h"
#include "mcs.h"

struct benchmark_case_config {
    int num_iters;
    int thread_count;
    int outside_work;
    int n;
};

static const benchmark_case_config BENCHMARK_CASES[] = {
    {1000, 20, 3, 100003},
    {1000, 16, 0, 50021},
    {1000, 20, 1, 10007},
};

// Change only this index to switch scenarios from the list above.
static constexpr std::size_t SELECTED_CASE_INDEX = 0;

static int is_prime(int p)
{
    for (int d = 2; d < p; ++d) {
        if (p % d == 0) {
            return 0;
        }
    }
    return 1;
}

static void tas_worker(tas_spinlock* mtx, int num_iters, int outside_work, int n)
{
    for (int i = 0; i < num_iters; ++i) {
        mtx->lock();
        assert(is_prime(n) == 1);
        mtx->unlock();

        for (int j = 0; j < outside_work; ++j) {
            assert(is_prime(n) == 1);
        }
    }
}

static void ttas_worker(ttas_spinlock* mtx, int num_iters, int outside_work, int n)
{
    for (int i = 0; i < num_iters; ++i) {
        mtx->lock();
        assert(is_prime(n) == 1);
        mtx->unlock();

        for (int j = 0; j < outside_work; ++j) {
            assert(is_prime(n) == 1);
        }
    }
}

static void mcs_worker(mcs_lock* mtx, int num_iters, int outside_work, int n)
{
    mcs_node node;
    for (int i = 0; i < num_iters; ++i) {
        mtx->lock(&node);
        assert(is_prime(n) == 1);
        mtx->unlock(&node);

        for (int j = 0; j < outside_work; ++j) {
            assert(is_prime(n) == 1);
        }
    }
}

struct pthread_worker_args {
    pthread_mutex_t* mtx;
    int num_iters;
    int outside_work;
    int n;
};

static void* pthread_worker(void* raw_args)
{
    auto* args = static_cast<pthread_worker_args*>(raw_args);
    for (int i = 0; i < args->num_iters; ++i) {
        pthread_mutex_lock(args->mtx);
        assert(is_prime(args->n) == 1);
        pthread_mutex_unlock(args->mtx);

        for (int j = 0; j < args->outside_work; ++j) {
            assert(is_prime(args->n) == 1);
        }
    }
    return nullptr;
}

static double time_tas_ms(int num_threads, int num_iters, int outside_work, int n, bool use_backoff)
{
    tas_spinlock mtx(use_backoff);
    std::vector<std::thread> threads(num_threads);

    const auto t0 = std::chrono::steady_clock::now();
    for (auto& th : threads) {
        th = std::thread(tas_worker, &mtx, num_iters, outside_work, n);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double time_ttas_ms(int num_threads, int num_iters, int outside_work, int n, bool use_backoff)
{
    ttas_spinlock mtx(use_backoff);
    std::vector<std::thread> threads(num_threads);

    const auto t0 = std::chrono::steady_clock::now();
    for (auto& th : threads) {
        th = std::thread(ttas_worker, &mtx, num_iters, outside_work, n);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double time_mcs_ms(int num_threads, int num_iters, int outside_work, int n)
{
    mcs_lock mtx;
    std::vector<std::thread> threads(num_threads);

    const auto t0 = std::chrono::steady_clock::now();
    for (auto& th : threads) {
        th = std::thread(mcs_worker, &mtx, num_iters, outside_work, n);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double time_pthread_mutex_ms(int num_threads, int num_iters, int outside_work, int n)
{
    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, nullptr);

    std::vector<pthread_t> threads(static_cast<size_t>(num_threads));
    std::vector<pthread_worker_args> args(static_cast<size_t>(num_threads), {&mtx, num_iters, outside_work, n});

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[static_cast<size_t>(i)], nullptr, pthread_worker, &args[static_cast<size_t>(i)]);
    }
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[static_cast<size_t>(i)], nullptr);
    }
    const auto t1 = std::chrono::steady_clock::now();

    pthread_mutex_destroy(&mtx);
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double time_omp_lock_ms(int num_threads, int num_iters, int outside_work, int n)
{
    omp_lock_t lock;
    omp_init_lock(&lock);

    const auto t0 = std::chrono::steady_clock::now();
#pragma omp parallel num_threads(num_threads) default(none) shared(lock, num_iters, outside_work, n)
    {
        for (int i = 0; i < num_iters; ++i) {
            omp_set_lock(&lock);
            assert(is_prime(n) == 1);
            omp_unset_lock(&lock);

            for (int j = 0; j < outside_work; ++j) {
                assert(is_prime(n) == 1);
            }
        }
    }
    const auto t1 = std::chrono::steady_clock::now();

    omp_destroy_lock(&lock);
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main()
{
    static_assert(SELECTED_CASE_INDEX < (sizeof(BENCHMARK_CASES) / sizeof(BENCHMARK_CASES[0])));
    const benchmark_case_config& selected_case = BENCHMARK_CASES[SELECTED_CASE_INDEX];

    std::cout << "threads,num_iters,outside_work,n,"
              << "tas_ms,tas_backoff_ms,"
              << "ttas_ms,ttas_backoff_ms,"
              << "mcs_ms,pthread_mutex_ms,omp_lock_ms\n";

    const double tas_ms = time_tas_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n, false
    );
    const double tas_backoff_ms = time_tas_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n, true
    );
    const double ttas_ms = time_ttas_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n, false
    );
    const double ttas_backoff_ms = time_ttas_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n, true
    );
    const double mcs_ms = time_mcs_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n
    );
    const double pthread_mutex_ms = time_pthread_mutex_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n
    );
    const double omp_lock_ms = time_omp_lock_ms(
        selected_case.thread_count, selected_case.num_iters, selected_case.outside_work, selected_case.n
    );

    std::cout << selected_case.thread_count << ',' << selected_case.num_iters << ','
              << selected_case.outside_work << ',' << selected_case.n << ','
              << tas_ms << ',' << tas_backoff_ms << ','
              << ttas_ms << ',' << ttas_backoff_ms << ','
              << mcs_ms << ',' << pthread_mutex_ms << ',' << omp_lock_ms << '\n';

    return 0;
}
