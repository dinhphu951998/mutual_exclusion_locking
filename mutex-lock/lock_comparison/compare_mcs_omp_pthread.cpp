#include <iostream>

#include "common.h"
#include "../mcs.h"

static constexpr int NUM_OF_ITERS = 1000;
static constexpr int THREAD_COUNT = 16;
static constexpr int OUTSIDE_WORK = 3;
static constexpr int N = 53;

static double time_mcs_ms()
{
    mcs_lock lock;
    return lock_comparison::time_std_threads(
        THREAD_COUNT,
        [&lock]() {
            mcs_node node;
            for (int i = 0; i < NUM_OF_ITERS; ++i) {
                lock.lock(&node);
                assert(lock_comparison::is_prime(N) == 1);
                lock.unlock(&node);

                for (int j = 0; j < OUTSIDE_WORK; ++j) {
                    assert(lock_comparison::is_prime(N) == 1);
                }
            }
        }
    );
}

int main()
{
    const double mcs_ms = time_mcs_ms();

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

    std::cout << "threads,num_iters,outside_work,n,mcs_ms,pthread_mutex_ms,omp_lock_ms\n";
    std::cout << THREAD_COUNT << ',' << NUM_OF_ITERS << ','
              << OUTSIDE_WORK << ',' << N << ','
              << mcs_ms << ',' << pthread_mutex_ms << ',' << omp_lock_ms << '\n';

    return 0;
}
