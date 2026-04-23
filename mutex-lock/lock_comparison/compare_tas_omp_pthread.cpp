#include <iostream>

#include "common.h"
#include "../tas.h"

static constexpr int NUM_OF_ITERS = 1000;
static constexpr int THREAD_COUNTS[] = {16};
static constexpr int OUTSIDE_WORKS[] = {0, 1 ,3 , 5, 7, 9};
static constexpr int NS[] = {100003};

static double time_tas_ms(int thread_count, int outside_work, int n, bool use_backoff)
{
    tas_spinlock lock(use_backoff);
    return lock_comparison::time_std_threads_lock(
        &lock,
        thread_count,
        NUM_OF_ITERS,
        outside_work,
        n
    );
}

int main()
{
    std::cout << "threads,num_iters,outside_work,n,tas_ms,tas_backoff_ms,pthread_mutex_ms,omp_lock_ms\n";

    for (int thread_count : THREAD_COUNTS) {
        for (int outside_work : OUTSIDE_WORKS) {
            for (int n : NS) {
                const double tas_ms = time_tas_ms(thread_count, outside_work, n, false);
                const double tas_backoff_ms = time_tas_ms(thread_count, outside_work, n, true);

                lock_comparison::pthread_mutex_wrapper pthread_lock;
                const double pthread_mutex_ms = lock_comparison::time_pthread_lock(
                    &pthread_lock,
                    thread_count,
                    NUM_OF_ITERS,
                    outside_work,
                    n
                );

                lock_comparison::omp_lock_wrapper omp_lock;
                const double omp_lock_ms = lock_comparison::time_openmp_lock(
                    &omp_lock,
                    thread_count,
                    NUM_OF_ITERS,
                    outside_work,
                    n
                );

                std::cout << thread_count << ',' << NUM_OF_ITERS << ','
                          << outside_work << ',' << n << ','
                          << tas_ms << ',' << tas_backoff_ms << ','
                          << pthread_mutex_ms << ',' << omp_lock_ms << '\n';
            }
        }
    }

    return 0;
}
