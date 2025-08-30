#include <catch2/catch_test_macros.hpp>
#include "octoweave/chunk_grid.hpp"

using namespace octoweave;

TEST_CASE("ChunkGrid basic mapping") {
  ChunkGrid g(2, AABB{0, 2, 0, 2, 0, 2});
  auto [ix,iy,iz,idx] = g.which(0.1, 0.1, 0.1);
  REQUIRE(ix == 0); REQUIRE(iy == 0); REQUIRE(iz == 0); REQUIRE(idx == 0);
  auto [ix2,iy2,iz2] = g.unravel(idx);
  REQUIRE(ix2 == ix); REQUIRE(iy2 == iy); REQUIRE(iz2 == iz);
}

TEST_CASE("ChunkGrid boundary clamping and boxes") {
  ChunkGrid g(2, AABB{0, 2, 0, 2, 0, 2});
  // Exactly on max edge should clamp to last tile
  auto [ix,iy,iz,idx] = g.which(2.0, 2.0, 2.0);
  REQUIRE(ix == 1); REQUIRE(iy == 1); REQUIRE(iz == 1);
  auto box = g.chunk_box(ix,iy,iz);
  REQUIRE(box.xmin == 1.0); REQUIRE(box.xmax == 2.0);
  REQUIRE(box.ymin == 1.0); REQUIRE(box.ymax == 2.0);
  REQUIRE(box.zmin == 1.0); REQUIRE(box.zmax == 2.0);
  // Below min should clamp to 0
  auto [ix2,iy2,iz2,idx2] = g.which(-0.5, -0.5, -0.5);
  REQUIRE(ix2 == 0); REQUIRE(iy2 == 0); REQUIRE(iz2 == 0); (void)idx2;
}

TEST_CASE("ChunkGrid tile coverage and centers") {
  ChunkGrid g(4, AABB{0, 4, 0, 4, 0, 4});
  // Check that cell centers map back to their indices
  for (int iz=0; iz<g.n(); ++iz) {
    for (int iy=0; iy<g.n(); ++iy) {
      for (int ix=0; ix<g.n(); ++ix) {
        auto b = g.chunk_box(ix,iy,iz);
        double cx = 0.5*(b.xmin + b.xmax);
        double cy = 0.5*(b.ymin + b.ymax);
        double cz = 0.5*(b.zmin + b.zmax);
        auto [rix,riy,riz, ridx] = g.which(cx,cy,cz);
        REQUIRE(rix == ix); REQUIRE(riy == iy); REQUIRE(riz == iz);
        auto [uix, uiy, uiz] = g.unravel(ridx);
        REQUIRE(uix == ix); REQUIRE(uiy == iy); REQUIRE(uiz == iz);
      }
    }
  }
}
