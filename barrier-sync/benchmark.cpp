#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <pthread.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "sense_reversing_barrier.h"

namespace {

constexpr int NUM_EPISODES = 10 * 1000;
constexpr int THREAD_COUNTS[] = {24};

double episodes_per_ms(int episodes, double elapsed_ms) {
  if (elapsed_ms <= 0.0) {
    return 0.0;
  }
  return episodes * 1.0 / elapsed_ms;
}

double time_sense_reversing_barrier(int threads, int episodes,
                                    bool use_backoff) {
  sense_reversing_barrier barrier(threads, use_backoff);

  auto worker = [&barrier, episodes]() {
    for (int i = 0; i < episodes; ++i) {
      barrier.arrive_and_wait();
    }
  };

  std::vector<std::thread> workers;
  workers.reserve(static_cast<size_t>(threads));

  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < threads; ++i) {
    workers.emplace_back(worker);
  }
  for (auto &worker_thread : workers) {
    worker_thread.join();
  }
  const auto t1 = std::chrono::steady_clock::now();

  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  return episodes_per_ms(episodes, elapsed_ms);
}

struct pthread_worker_args {
  pthread_barrier_t *barrier;
  int episodes;
};

void *pthread_worker(void *raw_args) {
  auto *args = static_cast<pthread_worker_args *>(raw_args);
  for (int i = 0; i < args->episodes; ++i) {
    pthread_barrier_wait(args->barrier);
  }
  return nullptr;
}

double time_pthread_barrier(int threads, int episodes) {
  pthread_barrier_t barrier{};
  pthread_barrier_init(&barrier, nullptr, static_cast<unsigned>(threads));

  std::vector<pthread_t> workers(static_cast<size_t>(threads));
  std::vector<pthread_worker_args> args(
      static_cast<size_t>(threads), pthread_worker_args{&barrier, episodes});

  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < threads; ++i) {
    pthread_create(&workers[static_cast<size_t>(i)], nullptr, pthread_worker,
                   &args[static_cast<size_t>(i)]);
  }
  for (int i = 0; i < threads; ++i) {
    pthread_join(workers[static_cast<size_t>(i)], nullptr);
  }
  const auto t1 = std::chrono::steady_clock::now();

  pthread_barrier_destroy(&barrier);

  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  return episodes_per_ms(episodes, elapsed_ms);
}

double time_openmp_barrier(int threads, int episodes) {
  const auto t0 = std::chrono::steady_clock::now();

#pragma omp parallel num_threads(threads) default(none) shared(episodes)
  {
    for (int i = 0; i < episodes; ++i) {
#pragma omp barrier
    }
  }

  const auto t1 = std::chrono::steady_clock::now();
  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  return episodes_per_ms(episodes, elapsed_ms);
}

} // namespace

int main() {
  int repeat = 20;

  std::cout << "threads,episodes,sense_reversing_barrier_eps_per_ms,"
            << "sense_reversing_barrier_eps_per_ms_with_eb,"
            << "pthread_eps_per_ms,openmp_eps_per_ms\n";

  for (int threads : THREAD_COUNTS) {
    for (int r = 0; r < repeat; ++r) {
      const double sense_reversing_barrier_ms =
          time_sense_reversing_barrier(threads, NUM_EPISODES, false);
      const double sense_reversing_barrier_ms_with_eb =
          time_sense_reversing_barrier(threads, NUM_EPISODES, true);
      const double pthread_ms = time_pthread_barrier(threads, NUM_EPISODES);
      const double openmp_ms = time_openmp_barrier(threads, NUM_EPISODES);

      std::cout << threads << ',' << NUM_EPISODES << ','
                << sense_reversing_barrier_ms << ','
                << sense_reversing_barrier_ms_with_eb << ',' << pthread_ms
                << ',' << openmp_ms << '\n';
    }
  }

  return 0;
}
