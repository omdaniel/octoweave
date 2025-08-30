#include <catch2/catch_test_macros.hpp>
#include "octoweave/chunk_grid.hpp"
#include "octoweave/octo_iface.hpp"
#include "octoweave/hierarchy.hpp"
#include "octoweave/parallel.hpp"
#include "octoweave/p8est_builder.hpp"
#include "octoweave/viz.hpp"
#include <vector>
#include <fstream>
#include <filesystem>

using namespace octoweave;

TEST_CASE("End-to-end: points -> chunks -> hierarchy -> forest -> viz") {
  // 1) Chunking
  ChunkGrid grid(2, AABB{0,4, 0,4, 0,4});
  std::vector<std::vector<Pt>> per_chunk(8);
  for (int iz=0; iz<2; ++iz) for (int iy=0; iy<2; ++iy) for (int ix=0; ix<2; ++ix) {
    auto B = grid.chunk_box(ix,iy,iz);
    // Place two points in distinct voxels within this chunk
    per_chunk[ix + 2*(iy + 2*iz)] = {
      {B.xmin + 0.1, B.ymin + 0.1, B.zmin + 0.1},
      {B.xmin + 1.1, B.ymin + 0.1, B.zmin + 0.1}
    };
  }

  // 2) Per-chunk trees (parallel)
  OctoChunker::Params params;
  auto outs = parallel_build_workers((int)per_chunk.size(), [&](int i){ return OctoChunker::build_and_export(per_chunk[i], params); }, 4);

  // 3) Hierarchy bottom-up union
  auto H = make_hierarchy_from_workers(outs, /*tau=*/0.5, /*use_logodds=*/false, /*p_unknown=*/0.5, /*base_depth=*/1);
  REQUIRE(H.td > 0);
  size_t leaf_count = 0; for (auto& kv : H.nodes) if (kv.second.is_leaf) ++leaf_count;
  REQUIRE(leaf_count >= 8); // should have at least one leaf per chunk

  // 4) p8est mapping (stub)
  P8estBuilder::prepare_want_sets(H, P8estBuilder::Config{2});

  // 5) Viz: export leaves to CSV and render one slice
  std::filesystem::create_directories("e2e_tmp");
  std::string csv = "e2e_tmp/leaves.csv";
  std::ofstream f(csv);
  int slice_z = -1; int slice_depth = H.td;
  for (auto& kv : H.nodes) if (kv.second.is_leaf) {
    auto k = kv.first.k; int d = kv.first.d; double p = kv.second.p;
    f << k.x << "," << k.y << "," << k.z << "," << d << "," << p << "\n";
    if (slice_z < 0 && d == slice_depth) slice_z = (int)k.z;
  }
  f.close(); if (slice_z < 0) slice_z = 0;

  std::string out = "e2e_tmp/slice.pgm";
  std::string s_slice = std::to_string(slice_z);
  std::string s_depth = std::to_string(slice_depth);
  std::vector<const char*> argv {
    "octoweave_viz", "--csv", csv.c_str(), "--slice_z", s_slice.c_str(), "--depth", s_depth.c_str(), "--out", out.c_str()
  };
  int rc = viz_main((int)argv.size(), const_cast<char**>(argv.data()));
  REQUIRE(rc == 0);
  // Load PGM and assert it has non-zero pixels
  std::ifstream pgm(out);
  REQUIRE((bool)pgm);
  std::string magic; int W,H, maxv; pgm >> magic >> W >> H >> maxv;
  REQUIRE(magic == "P2");
  int sum=0; for (int i=0;i<W*H;++i){ int v; pgm >> v; sum += v; }
  REQUIRE(sum > 0);
}
