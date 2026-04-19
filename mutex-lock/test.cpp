#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "tas.h"
#include "ttas.h"
#include "mcs.h"

static void worker_tas(tas_spinlock* mtx, int* counter, int increments_per_thread)
{
    for (int i = 0; i < increments_per_thread; ++i) {
        mtx->lock();
        ++(*counter);
        mtx->unlock();
    }
}

static void worker_ttas(ttas_spinlock* mtx, int* counter, int increments_per_thread)
{
    for (int i = 0; i < increments_per_thread; ++i) {
        mtx->lock();
        ++(*counter);
        mtx->unlock();
    }
}

static void worker_mcs(mcs_lock* mtx, int* counter, int increments_per_thread)
{
    mcs_node node;
    for (int i = 0; i < increments_per_thread; ++i) {
        mtx->lock(&node);
        ++(*counter);
        mtx->unlock(&node);
    }
}

static bool run_tas(int num_threads, int increments_per_thread)
{
    tas_spinlock mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker_tas, &mtx, &counter, increments_per_thread);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int expected = num_threads * increments_per_thread;
    if (counter != expected) {
        std::cerr << "TAS FAIL: counter=" << counter << " expected=" << expected << " (threads="
                  << num_threads << " inc/thread=" << increments_per_thread << ")\n";
        return false;
    }
    std::cout << "TAS OK: counter=" << counter << " (" << num_threads << " * " << increments_per_thread
              << ")\n";
    return true;
}

static bool run_ttas(int num_threads, int increments_per_thread)
{
    ttas_spinlock mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker_ttas, &mtx, &counter, increments_per_thread);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int expected = num_threads * increments_per_thread;
    if (counter != expected) {
        std::cerr << "TTAS FAIL: counter=" << counter << " expected=" << expected << " (threads="
                  << num_threads << " inc/thread=" << increments_per_thread << ")\n";
        return false;
    }
    std::cout << "TTAS OK: counter=" << counter << " (" << num_threads << " * " << increments_per_thread
              << ")\n";
    return true;
}

static bool run_mcs(int num_threads, int increments_per_thread)
{
    mcs_lock mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker_mcs, &mtx, &counter, increments_per_thread);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int expected = num_threads * increments_per_thread;
    if (counter != expected) {
        std::cerr << "MCS FAIL: counter=" << counter << " expected=" << expected << " (threads="
                  << num_threads << " inc/thread=" << increments_per_thread << ")\n";
        return false;
    }
    std::cout << "MCS OK: counter=" << counter << " (" << num_threads << " * " << increments_per_thread
              << ")\n";
    return true;
}

int main()
{
    const int thread_counts[] = {1, 2, 4, 8, 16, 20};
    const int increments_per_thread[] = {100, 1000, 10000};

    bool all_ok = true;
    for (int num_threads : thread_counts) {
        for (int inc : increments_per_thread) {
            if (!run_tas(num_threads, inc)) {
                all_ok = false;
            }
            if (!run_ttas(num_threads, inc)) {
                all_ok = false;
            }
            if (!run_mcs(num_threads, inc)) {
                all_ok = false;
            }
        }
    }

    if (!all_ok) {
        std::cerr << "Some counter checks failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "All counter checks passed.\n";
    return EXIT_SUCCESS;
}
