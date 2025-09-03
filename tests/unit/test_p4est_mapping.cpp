#include <catch2/catch_test_macros.hpp>
#include "octoweave/p4est_builder.hpp"

using namespace octoweave;

TEST_CASE("p4est mapping stub: global (k,d) -> (tree, local)") {
  int n = 2;
  // depth d is unused in stub but included for API shape
  int d = 5;
  auto [t0, l0] = P4estBuilder::split_global_to_tree_local(Key3{0,0,0}, d, n);
  REQUIRE(t0.x==0 && t0.y==0 && t0.z==0);
  REQUIRE(l0.x==0 && l0.y==0 && l0.z==0);

  auto [t1, l1] = P4estBuilder::split_global_to_tree_local(Key3{3,1,0}, d, n);
  REQUIRE(t1.x==1 && t1.y==1 && t1.z==0);
  REQUIRE(l1.x==1 && l1.y==0 && l1.z==0);

  auto [t2, l2] = P4estBuilder::split_global_to_tree_local(Key3{5,7,9}, d, 4);
  REQUIRE(t2.x==1 && t2.y==3 && t2.z==1);
  REQUIRE(l2.x==1 && l2.y==1 && l2.z==2);
}

