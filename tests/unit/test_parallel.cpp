#include <catch2/catch_test_macros.hpp>
#include "octoweave/parallel.hpp"
#include <random>

using namespace octoweave;

static WorkerOut make_worker_from_seed(int seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> dist_k(0, 7);
  std::uniform_real_distribution<double> dist_p(0.0, 1.0);
  WorkerOut w; w.td = 4;
  int N = 50; // fixed size for determinism
  for (int i=0;i<N;++i) {
    Key3 k{ (uint32_t)dist_k(rng), (uint32_t)dist_k(rng), (uint32_t)dist_k(rng) };
    double p = dist_p(rng);
    double& slot = w.Ptd[k];
    slot = 1.0 - (1.0 - slot) * (1.0 - p);
  }
  return w;
}

TEST_CASE("parallel_build_workers determinism and race-free") {
  int chunks = 8;
  auto build = [&](int i){ return make_worker_from_seed(1234 + i); };
  auto A = parallel_build_workers(chunks, build, /*max_threads=*/4);
  auto B = parallel_build_workers(chunks, build, /*max_threads=*/2);
  REQUIRE(A.size() == (size_t)chunks);
  REQUIRE(B.size() == (size_t)chunks);
  for (int i=0;i<chunks;++i) {
    REQUIRE(A[i].td == B[i].td);
    REQUIRE(A[i].Ptd.size() == B[i].Ptd.size());
    for (auto& kv : A[i].Ptd) {
      auto it = B[i].Ptd.find(kv.first);
      REQUIRE(it != B[i].Ptd.end());
      REQUIRE(kv.second == Approx(it->second).epsilon(1e-12));
    }
  }
}

