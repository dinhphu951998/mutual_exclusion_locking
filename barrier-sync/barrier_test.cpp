#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "sense_reversing_barrier.h"

static constexpr int NUM_PHASES = 1000 * 1000;
static constexpr int THREAD_COUNTS[] = {2, 4, 8, 16, 20};

static bool run_barrier_test(int num_threads)
{
    sense_reversing_barrier barrier(num_threads, true);
    std::vector<std::atomic<int>> arrivals(static_cast<size_t>(NUM_PHASES));
    std::atomic<bool> ok{true};

    for (auto& arrival : arrivals) {
        arrival.store(0, std::memory_order_relaxed);
    }

    auto worker = [&barrier, &arrivals, &ok, num_threads]() {
        for (int phase = 0; phase < NUM_PHASES; ++phase) {
            arrivals[static_cast<size_t>(phase)].fetch_add(1, std::memory_order_relaxed);
            barrier.arrive_and_wait();

            const int arrived = arrivals[static_cast<size_t>(phase)].load(std::memory_order_acquire);
            if (arrived != num_threads) {
                ok.store(false, std::memory_order_relaxed);
            }

            barrier.arrive_and_wait();
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& thread : threads) {
        thread.join();
    }

    return ok.load(std::memory_order_relaxed);
}

int main()
{
    bool all_ok = true;

    for (int num_threads : THREAD_COUNTS) {
        const bool passed = run_barrier_test(num_threads);
        std::cout << "sense_reversing_barrier with " << num_threads << " threads: "
                  << (passed ? "OK" : "FAIL") << '\n';
        all_ok = all_ok && passed;
    }

    if (!all_ok) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
