#include <catch2/catch_test_macros.hpp>
#include "octoweave/hierarchy.hpp"

using namespace octoweave;

TEST_CASE("Hierarchy rollup and emission with tau") {
  WorkerOut w; w.td = 2;
  // Four children present under parent (0,0,0) at d=1
  w.Ptd[{0,0,0}] = 0.6; // i=0
  w.Ptd[{1,0,0}] = 0.6; // i=1
  w.Ptd[{0,1,0}] = 0.6; // i=2
  w.Ptd[{1,1,0}] = 0.6; // i=3
  auto H = make_hierarchy_from_workers({w}, 0.55, false, 0.5, 1);
  REQUIRE(H.td == 2);
  REQUIRE(H.base_depth == 1);
  // Parent at depth 1 should be internal (passes tau and has child evidence)
  NDKey ndp{ Key3{0,0,0}, (uint16_t)1 };
  REQUIRE(H.nodes.count(ndp) == 1);
  REQUIRE(H.nodes.at(ndp).is_leaf == false);
  // And children at depth 2 exist as leaves
  for (auto k : {Key3{0,0,0}, Key3{1,0,0}, Key3{0,1,0}, Key3{1,1,0}}) {
    NDKey ndc{ k, (uint16_t)2 };
    REQUIRE(H.nodes.count(ndc) == 1);
    REQUIRE(H.nodes.at(ndc).is_leaf == true);
  }
}

TEST_CASE("Hierarchy missing children use p_unknown in rollup") {
  WorkerOut w; w.td = 2;
  // Only one child provided: probability 0.0; others missing -> use p_unknown
  w.Ptd[{0,0,0}] = 0.0; // i=0 child of parent (0,0,0) at d=1
  double p_unknown = 0.25;
  auto H = make_hierarchy_from_workers({w}, 0.0, false, p_unknown, 1);
  NDKey ndp{ Key3{0,0,0}, (uint16_t)1 };
  REQUIRE(H.nodes.count(ndp) == 1);
  double expected = 1.0 - std::pow(1.0 - p_unknown, 7); // since one child has p=0
  REQUIRE(H.nodes.at(ndp).p == Approx(expected).epsilon(1e-12));
}
