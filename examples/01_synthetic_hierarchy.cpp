#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include "octoweave/chunk_grid.hpp"
#include "octoweave/octo_iface.hpp"
#include "octoweave/hierarchy.hpp"

int main() {
  using namespace octoweave;
  namespace fs = std::filesystem;
  std::cout << "[ex01] Building synthetic hierarchy...\n";

  // 1) Define a 2x2x2 chunk grid over a 4x4x4 world
  ChunkGrid grid(2, AABB{0,4, 0,4, 0,4});

  // 2) Generate simple per-chunk point clouds
  std::vector<std::vector<Pt>> per_chunk(8);
  for (int iz=0; iz<2; ++iz) for (int iy=0; iy<2; ++iy) for (int ix=0; ix<2; ++ix) {
    auto B = grid.chunk_box(ix,iy,iz);
    // Two points in distinct voxels
    per_chunk[ix + 2*(iy + 2*iz)] = {
      {B.xmin + 0.2, B.ymin + 0.2, B.zmin + 0.2},
      {B.xmin + 1.2, B.ymin + 0.2, B.zmin + 0.2}
    };
  }

  // 3) Build per-chunk results via OctoChunker
  OctoChunker::Params params;
  params.res = 0.25;       // tree resolution
  params.emit_res = 0.5;   // export at coarser resolution
  params.max_depth_cap = 12;
  std::vector<WorkerOut> outs; outs.reserve(per_chunk.size());
  for (size_t i=0;i<per_chunk.size();++i) {
    outs.push_back(OctoChunker::build_and_export(per_chunk[i], params));
  }

  // 4) Bottom-up hierarchy
  Hierarchy H = make_hierarchy_from_workers(outs, /*tau=*/0.5, /*use_logodds=*/false, /*p_unknown=*/0.5, /*base_depth=*/1);
  size_t leaves = 0; for (auto& kv : H.nodes) if (kv.second.is_leaf) ++leaves;
  std::cout << "[ex01] td=" << H.td << ", base=" << H.base_depth << ", leaves=" << leaves << "\n";

  // 5) Export leaves to CSV for viz
  fs::create_directories("examples_out");
  std::ofstream f("examples_out/leaves.csv");
  for (auto& kv : H.nodes) if (kv.second.is_leaf) {
    auto k = kv.first.k; int d = kv.first.d; double p = kv.second.p;
    f << k.x << "," << k.y << "," << k.z << "," << d << "," << p << "\n";
  }
  std::cout << "[ex01] Wrote examples_out/leaves.csv\n";
  return 0;
}

