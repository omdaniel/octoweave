#include "octoweave/parallel.hpp"
#include <thread>
#include <atomic>

namespace octoweave {

std::vector<WorkerOut> parallel_build_workers(int num_chunks,
                                              const std::function<WorkerOut(int)>& build,
                                              int max_threads)
{
  if (num_chunks <= 0) return {};
  if (max_threads <= 0) max_threads = (int)std::max(1u, std::thread::hardware_concurrency());
  std::vector<WorkerOut> out(num_chunks);
  std::atomic<int> next{0};
  int T = std::min(max_threads, num_chunks);
  std::vector<std::thread> threads; threads.reserve(T);
  for (int t=0;t<T;++t) {
    threads.emplace_back([&]{
      while (true) {
        int i = next.fetch_add(1);
        if (i >= num_chunks) break;
        out[i] = build(i);
      }
    });
  }
  for (auto& th : threads) th.join();
  return out;
}

} // namespace octoweave

