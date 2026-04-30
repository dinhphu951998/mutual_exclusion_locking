#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "tas.h"
#include "ttas.h"
#include "mcs.h"

static const int NUM_OF_ITERS = 100000; // keep it the same for every lock type
static const int THREAD_COUNTS[] = {1, 2, 4, 8, 16, 20};
static const int OUTSIDE_WORKS[] = {0, 1, 3, 5, 7};

static void do_outside_work(int outside_work)
{
    volatile int sink = 0;
    for (int j = 0; j < outside_work; ++j) {
        sink += j;
    }
}

static void worker_tas(tas_spinlock* mtx, int* counter, int outside_work)
{
    for (int i = 0; i < NUM_OF_ITERS; ++i) {
        mtx->lock();
        ++(*counter);
        mtx->unlock();
        do_outside_work(outside_work);
    }
}

static void worker_ttas(ttas_spinlock* mtx, int* counter, int outside_work)
{
    for (int i = 0; i < NUM_OF_ITERS; ++i) {
        mtx->lock();
        ++(*counter);
        mtx->unlock();
        do_outside_work(outside_work);
    }
}

static void worker_mcs(mcs_lock* mtx, int* counter, int outside_work)
{
    mcs_node node;
    for (int i = 0; i < NUM_OF_ITERS; ++i) {
        mtx->lock(&node);
        ++(*counter);
        mtx->unlock(&node);
        do_outside_work(outside_work);
    }
}

static bool run_tas(int num_threads, int outside_work)
{
    tas_spinlock mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker_tas, &mtx, &counter, outside_work);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int expected = num_threads * NUM_OF_ITERS;
    if (counter != expected) {
        std::cerr << "TAS FAIL: counter=" << counter << " expected=" << expected << " (threads="
                  << num_threads << " num_iters=" << NUM_OF_ITERS
                  << " outside_work=" << outside_work << ")\n";
        return false;
    }
    std::cout << "TAS OK: counter=" << counter << " (" << num_threads << " * " << NUM_OF_ITERS
              << ", outside_work=" << outside_work << ")\n";
    return true;
}

static bool run_ttas(int num_threads, int outside_work)
{
    ttas_spinlock mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker_ttas, &mtx, &counter, outside_work);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int expected = num_threads * NUM_OF_ITERS;
    if (counter != expected) {
        std::cerr << "TTAS FAIL: counter=" << counter << " expected=" << expected << " (threads="
                  << num_threads << " num_iters=" << NUM_OF_ITERS
                  << " outside_work=" << outside_work << ")\n";
        return false;
    }
    std::cout << "TTAS OK: counter=" << counter << " (" << num_threads << " * " << NUM_OF_ITERS
              << ", outside_work=" << outside_work << ")\n";
    return true;
}

static bool run_mcs(int num_threads, int outside_work)
{
    mcs_lock mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(worker_mcs, &mtx, &counter, outside_work);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int expected = num_threads * NUM_OF_ITERS;
    if (counter != expected) {
        std::cerr << "MCS FAIL: counter=" << counter << " expected=" << expected << " (threads="
                  << num_threads << " num_iters=" << NUM_OF_ITERS
                  << " outside_work=" << outside_work << ")\n";
        return false;
    }
    std::cout << "MCS OK: counter=" << counter << " (" << num_threads << " * " << NUM_OF_ITERS
              << ", outside_work=" << outside_work << ")\n";
    return true;
}

int main()
{
    bool all_ok = true;
    for (int num_threads : THREAD_COUNTS) {
        for (int outside_work : OUTSIDE_WORKS) {
            if (!run_tas(num_threads, outside_work)) {
                all_ok = false;
            }
            if (!run_ttas(num_threads, outside_work)) {
                all_ok = false;
            }
            if (!run_mcs(num_threads, outside_work)) {
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
