#include <iostream>

#include "common.h"
#include "../mcs.h"

static constexpr int NUM_OF_ITERS = 1000;
static constexpr int THREAD_COUNTS[] = {8, 12, 16, 20};
static constexpr int OUTSIDE_WORKS[] = {1, 3, 5, 7};
static constexpr int NS[] = {53, 89, 101, 503};

static double time_mcs_ms(int thread_count, int outside_work, int n)
{
    mcs_lock lock;
    return lock_comparison::time_std_threads(
        thread_count,
        [&lock, outside_work, n]() {
            mcs_node node;
            for (int i = 0; i < NUM_OF_ITERS; ++i) {
                lock.lock(&node);
                assert(lock_comparison::is_prime(n) == 1);
                lock.unlock(&node);

                for (int j = 0; j < outside_work; ++j) {
                    assert(lock_comparison::is_prime(n) == 1);
                }
            }
        }
    );
}

int main()
{
    std::cout << "threads,num_iters,outside_work,n,mcs_ms,pthread_mutex_ms,omp_lock_ms\n";
    for (int thread_count : THREAD_COUNTS) {
        for (int outside_work : OUTSIDE_WORKS) {
            for (int n : NS) {
                const double mcs_ms = time_mcs_ms(thread_count, outside_work, n);

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
                          << mcs_ms << ',' << pthread_mutex_ms << ',' << omp_lock_ms << '\n';
            }
        }
    }

    return 0;
}
