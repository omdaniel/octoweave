#include <catch2/catch_test_macros.hpp>
#include "octoweave/octo_iface.hpp"

using namespace octoweave;

TEST_CASE("OctoChunker stub: chunk -> leaves at forced td") {
  std::vector<Pt> pts = {
    {0.2, 0.2, 0.2},
    {0.25, 0.2, 0.2}, // same voxel as above
    {1.1, 0.2, 0.2}   // neighbor voxel in +x
  };
  OctoChunker::Params p; // defaults
  auto w = OctoChunker::build_and_export(pts, p);
  REQUIRE(w.td == 8);
  REQUIRE(w.Ptd.size() == 2);
  // Expected per-voxel accumulations with p1 = 0.7
  Key3 a{0,0,0}; Key3 b{1,0,0};
  REQUIRE(w.Ptd[a] == Approx(1.0 - (1.0-0.7)*(1.0-0.7)));
  REQUIRE(w.Ptd[b] == Approx(0.7));

  auto tree = make_stub_tree_from_worker(w);
  REQUIRE(tree->max_depth() == w.td);
  auto leaves = tree->export_leaves_at_depth(w.td);
  REQUIRE(leaves.size() == 2);
  // Confirm the same key-prob pairs appear
  bool seen_a=false, seen_b=false;
  for (auto& t : leaves) {
    auto k = std::get<0>(t);
    auto pr = std::get<1>(t);
    if (k.x==a.x && k.y==a.y && k.z==a.z) { REQUIRE(pr == Approx(w.Ptd[a])); seen_a=true; }
    if (k.x==b.x && k.y==b.y && k.z==b.z) { REQUIRE(pr == Approx(w.Ptd[b])); seen_b=true; }
  }
  REQUIRE(seen_a && seen_b);
}

