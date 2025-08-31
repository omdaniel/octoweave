#include <iostream>
#include <random>
#include <vector>
#include "octoweave/chunk_grid.hpp"
#include "octoweave/octo_iface.hpp"
#include "octoweave/parallel.hpp"

int main(){
  using namespace octoweave;
  std::cout << "[ex05] Parallel per-chunk build determinism demo...\n";

  // 1) 4x4x4 chunks over 8x8x8 world
  int n = 4; ChunkGrid grid(n, AABB{0,8, 0,8, 0,8});
  std::vector<std::vector<Pt>> per_chunk(n*n*n);

  // Seeded synthetic points per chunk
  for (int iz=0; iz<n; ++iz) for (int iy=0; iy<n; ++iy) for (int ix=0; ix<n; ++ix) {
    auto B = grid.chunk_box(ix,iy,iz);
    int idx = ix + n*(iy + n*iz);
    std::mt19937 rng(1234 + idx);
    std::uniform_real_distribution<double> ux(0.0, (B.xmax - B.xmin));
    std::uniform_real_distribution<double> uy(0.0, (B.ymax - B.ymin));
    std::uniform_real_distribution<double> uz(0.0, (B.zmax - B.zmin));
    int pts = 8;
    per_chunk[idx].reserve(pts);
    for (int k=0;k<pts;++k) per_chunk[idx].push_back({B.xmin + ux(rng), B.ymin + uy(rng), B.zmin + uz(rng)});
  }

  OctoChunker::Params p; p.res = 0.25; p.emit_res = 0.5; p.max_depth_cap = 12;
  auto build = [&](int i){ return OctoChunker::build_and_export(per_chunk[i], p); };

  auto A = parallel_build_workers((int)per_chunk.size(), build, /*max_threads=*/2);
  auto B = parallel_build_workers((int)per_chunk.size(), build, /*max_threads=*/6);

  bool same = true;
  for (size_t i=0;i<A.size();++i) {
    same = same && (A[i].td == B[i].td) && (A[i].Ptd.size() == B[i].Ptd.size());
    if (!same) break;
    for (auto& kv : A[i].Ptd) {
      auto it = B[i].Ptd.find(kv.first);
      if (it == B[i].Ptd.end()) { same = false; break; }
      if (std::abs(kv.second - it->second) > 1e-12) { same = false; break; }
    }
    if (!same) break;
  }
  std::cout << "[ex05] Deterministic across threads: " << (same ? "YES" : "NO") << "\n";
  return same ? 0 : 1;
}

