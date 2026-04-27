#include <iostream>

#include "common.h"
#include "../ttas.h"

static constexpr int NUM_OF_ITERS = 1000;
static constexpr int THREAD_COUNTS[] = {8, 12, 16, 20};
static constexpr int OUTSIDE_WORKS[] = {0};
static constexpr int NS[] = {53, 89, 101, 503, 1009};

static double time_ttas_ms(int thread_count, int outside_work, int n, bool use_backoff)
{
    ttas_spinlock lock(use_backoff);
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
    std::cout << "threads,num_iters,outside_work,n,ttas_ms,ttas_backoff_ms,pthread_mutex_ms,omp_lock_ms\n";
    for (int thread_count : THREAD_COUNTS) {
        for (int outside_work : OUTSIDE_WORKS) {
            for (int n : NS) {
                const double ttas_ms = time_ttas_ms(thread_count, outside_work, n, false);
                const double ttas_backoff_ms = time_ttas_ms(thread_count, outside_work, n, true);

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
                          << ttas_ms << ',' << ttas_backoff_ms << ','
                          << pthread_mutex_ms << ',' << omp_lock_ms << '\n';
            }
        }
    }

    return 0;
}
