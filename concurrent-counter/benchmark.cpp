#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "concurrent_counters.h"

template <typename Counter>
double run_benchmark(const char *name, int num_threads,
                     int increments_per_thread, bool use_backoff = false) {
  Counter counter(use_backoff);
  std::vector<std::thread> threads;
  threads.reserve(static_cast<size_t>(num_threads));
  const long long total_increments =
      static_cast<long long>(increments_per_thread) * num_threads;

  auto start = std::chrono::steady_clock::now();
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&counter, increments_per_thread]() {
      for (int i = 0; i < increments_per_thread; ++i) {
        counter.increment();
      }
    });
  }
  for (auto &th : threads) {
    th.join();
  }
  auto end = std::chrono::steady_clock::now();

  assert(total_increments == counter.get());

  std::chrono::duration<double, std::milli> elapsed = end - start;
  return elapsed.count();
}

int main() {
  const int THREAD_COUNTS[] = {1, 2, 4, 8, 12, 16, 20};
  const int INCREMENTS_PER_THREAD[] = {100'000};
  int repeat = 20;

  std::cout << "threads,increments_per_thread,"
            << "mutex_ms,mutex_eb_ms,"
            << "cas_ms,cas_eb_ms,"
            << "fetch_add_ms\n";
  while (repeat-- > 0) {
    for (int num_threads : THREAD_COUNTS) {
      for (int increments_per_thread : INCREMENTS_PER_THREAD) {

        const double cas_ms = run_benchmark<CASCounter>(
            "CASCounter", num_threads, increments_per_thread, false);
        const double cas_eb_ms = run_benchmark<CASCounter>(
            "CASCounterWithEB", num_threads, increments_per_thread, true);

        const double mutex_ms = run_benchmark<MutexCounter>(
            "MutexCounter", num_threads, increments_per_thread, false);
        const double mutex_eb_ms = run_benchmark<MutexCounter>(
            "MutexCounterWithEB", num_threads, increments_per_thread, true);

        const double faa_ms = run_benchmark<FetchAddCounter>(
            "FetchAddCounter", num_threads, increments_per_thread, false);

        std::cout << num_threads << ',' << increments_per_thread << ','
                  << mutex_ms << ',' << mutex_eb_ms << ',' << cas_ms << ','
                  << cas_eb_ms << ',' << faa_ms << '\n';
      }
    }
  }

  return 0;
}
