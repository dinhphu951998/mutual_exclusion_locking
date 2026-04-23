#include <iostream>

#include "common.h"
#include "../ttas.h"

static constexpr int NUM_OF_ITERS = 1000;
static constexpr int THREAD_COUNT = 16;
static constexpr int OUTSIDE_WORK = 0;
static constexpr int N = 101;

static double time_ttas_ms(bool use_backoff)
{
    ttas_spinlock lock(use_backoff);
    return lock_comparison::time_std_threads_lock(
        &lock,
        THREAD_COUNT,
        NUM_OF_ITERS,
        OUTSIDE_WORK,
        N
    );
}

int main()
{
    const double ttas_ms = time_ttas_ms(false);
    const double ttas_backoff_ms = time_ttas_ms(true);

    lock_comparison::pthread_mutex_wrapper pthread_lock;
    const double pthread_mutex_ms = lock_comparison::time_pthread_lock(
        &pthread_lock,
        THREAD_COUNT,
        NUM_OF_ITERS,
        OUTSIDE_WORK,
        N
    );

    lock_comparison::omp_lock_wrapper omp_lock;
    const double omp_lock_ms = lock_comparison::time_openmp_lock(
        &omp_lock,
        THREAD_COUNT,
        NUM_OF_ITERS,
        OUTSIDE_WORK,
        N
    );

    std::cout << "threads,num_iters,outside_work,n,ttas_ms,ttas_backoff_ms,pthread_mutex_ms,omp_lock_ms\n";
    std::cout << THREAD_COUNT << ',' << NUM_OF_ITERS << ','
              << OUTSIDE_WORK << ',' << N << ','
              << ttas_ms << ',' << ttas_backoff_ms << ','
              << pthread_mutex_ms << ',' << omp_lock_ms << '\n';

    return 0;
}
