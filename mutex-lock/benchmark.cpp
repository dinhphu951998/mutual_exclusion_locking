#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "tas.h"
#include "ttas.h"
#include "mcs.h"

static const int NUM_OF_ITERS = 1000; // keep it the same for every locking, easier to analyze the result
static const int THREAD_COUNTS[] = {1, 2, 4, 8, 12, 16, 20};
static const int OUTSIDE_WORKS[] = {0, 1, 3, 5, 7, 9};
static const int NS[] = {53, 89, 101, 503, 1009, 10007, 50021, 100003};
// static const int NS[] = { 101, 1009, 10007};
static const bool USE_BACKOFF_OPTIONS[] = {true};

using namespace std;

int is_prime(int p)
{
    int d;
    for (d = 2; d < p; d++) {
        if (p % d == 0) {
            return 0;
        }
    }
    return 1;
}

static void tas_worker(tas_spinlock* mtx, int outside_work, int n)
{
    for (int i = 0; i < NUM_OF_ITERS; i++) {
        mtx->lock();
        assert(1 == is_prime(n));
        mtx->unlock();
        for (int j = 0; j < outside_work; j++) {
            assert(1 == is_prime(n));
        }
    }
}

static void ttas_worker(ttas_spinlock* mtx, int outside_work, int n)
{
    for (int i = 0; i < NUM_OF_ITERS; i++) {
        mtx->lock();
        assert(1 == is_prime(n));
        mtx->unlock();
        for (int j = 0; j < outside_work; j++) {
            assert(1 == is_prime(n));
        }
    }
}

static void mcs_worker(mcs_lock* mtx, int outside_work, int n)
{
    mcs_node node;
    for (int i = 0; i < NUM_OF_ITERS; i++) {
        mtx->lock(&node);
        assert(1 == is_prime(n));
        mtx->unlock(&node);
        for (int j = 0; j < outside_work; j++) {
            assert(1 == is_prime(n));
        }
    }
}

static double time_tas_ms(int num_threads, int outside_work, int n, bool use_backoff)
{
    tas_spinlock mtx(use_backoff);

    vector<thread> threads(num_threads);
    const auto t0 = chrono::steady_clock::now();
    for (auto& th : threads) {
        th = thread(tas_worker, &mtx, outside_work, n);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = chrono::steady_clock::now();
    return chrono::duration<double, milli>(t1 - t0).count();
}

static double time_ttas_ms(int num_threads, int outside_work, int n, bool use_backoff)
{
    ttas_spinlock mtx(use_backoff);

    vector<thread> threads(num_threads);
    const auto t0 = chrono::steady_clock::now();
    for (auto& th : threads) {
        th = thread(ttas_worker, &mtx, outside_work, n);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = chrono::steady_clock::now();
    return chrono::duration<double, milli>(t1 - t0).count();
}

static double time_mcs_ms(int num_threads, int outside_work, int n)
{
    mcs_lock mtx;

    vector<thread> threads(num_threads);
    const auto t0 = chrono::steady_clock::now();
    for (auto& th : threads) {
        th = thread(mcs_worker, &mtx, outside_work, n);
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto t1 = chrono::steady_clock::now();
    return chrono::duration<double, milli>(t1 - t0).count();
}

int main()
{
    constexpr size_t n_tc = size(THREAD_COUNTS);
    constexpr size_t n_ow = size(OUTSIDE_WORKS);
    constexpr size_t n_n = size(NS);
    constexpr size_t n_bo = size(USE_BACKOFF_OPTIONS);

    cout << "threads,num_iters,outside_work,n,use_backoff,tas_ms,ttas_ms,mcs_ms\n";

    for (int i = 0; i < 20; i++) {

        for (size_t d = 0; d < n_bo; ++d) {
            bool use_backoff = USE_BACKOFF_OPTIONS[d];
            for (size_t a = 0; a < n_tc; ++a) {
                int num_threads = THREAD_COUNTS[a];
                if (num_threads <= 0) {
                    continue;
                }
                for (size_t b = 0; b < n_ow; ++b) {
                    int outside_work = OUTSIDE_WORKS[b];
                    for (size_t c = 0; c < n_n; ++c) {
                        int n = NS[c];
    
                        double tas_ms = time_tas_ms(num_threads, outside_work, n, use_backoff);
                        double ttas_ms = time_ttas_ms(num_threads, outside_work, n, use_backoff);
                        double mcs_ms = use_backoff ? -1 : time_mcs_ms(num_threads, outside_work, n);
                        cout << num_threads << ',' << NUM_OF_ITERS << ',' << outside_work
                             << ',' << n << ',' << (use_backoff ? 1 : 0) << ','
                             << tas_ms << ',' << ttas_ms << ',' << mcs_ms << '\n';
                    }
                }
            }
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }
    

    return 0;
}
