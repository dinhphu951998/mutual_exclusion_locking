#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "concurrent_counters.h"

template <typename Counter>
bool run_counter_test(const char* name, int num_threads, int increments_per_thread, bool use_backoff = false)
{
    Counter counter(use_backoff);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&counter, increments_per_thread]() {
            for (int i = 0; i < increments_per_thread; ++i) {
                counter.increment();
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    const long long expected = increments_per_thread * num_threads;
    const long long actual = counter.get();
    if (actual != expected) {
        std::cerr << "[FAIL] " << name
                  << " threads=" << num_threads
                  << " increments_per_thread=" << increments_per_thread
                  << " use_backoff=" << use_backoff
                  << " actual=" << actual
                  << " expected=" << expected << '\n';
        return false;
    }

    return true;
}

int main()
{
    const int THREAD_COUNTS[] = {1, 2, 4, 8, 16, 20};
    const int INCREMENTS_PER_THREAD[] = {10'000, 100'000, 1'000'000};
    bool all_ok = true;

    for (int num_threads : THREAD_COUNTS) {
        for (int increments_per_thread : INCREMENTS_PER_THREAD) {
            const bool mutex_ok = run_counter_test<MutexCounter>(
                "MutexCounter", num_threads, increments_per_thread, false
            );
            const bool mutex_backoff_ok = run_counter_test<MutexCounter>(
                "MutexCounterWithEB", num_threads, increments_per_thread, true
            );
            const bool cas_ok = run_counter_test<CASCounter>(
                "CASCounter", num_threads, increments_per_thread, false
            );
            const bool cas_backoff_ok = run_counter_test<CASCounter>(
                "CASCounterWithEB", num_threads, increments_per_thread, true
            );
            const bool faa_ok = run_counter_test<FetchAddCounter>(
                "FetchAddCounter", num_threads, increments_per_thread, false
            );

            all_ok = all_ok && mutex_ok && mutex_backoff_ok
                            && cas_ok && cas_backoff_ok
                            && faa_ok;
        }
    }

    if (!all_ok) {
        std::cerr << "Some counter tests failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All counter tests with and without exponential backoff passed.\n";
    return EXIT_SUCCESS;
}
