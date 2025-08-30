#pragma once
#include <tuple>
#include <cstdint>
#include <algorithm>

namespace octoweave {

struct AABB {
  double xmin, xmax;
  double ymin, ymax;
  double zmin, zmax;
};

class ChunkGrid {
public:
  ChunkGrid(int n, AABB box);

  int n() const noexcept { return n_; }
  const AABB& box() const noexcept { return box_; }

  // Returns (ix,iy,iz, linear_index)
  std::tuple<int,int,int,int> which(double x, double y, double z) const;

  // Axis-aligned bounds of a chunk (ix,iy,iz)
  AABB chunk_box(int ix, int iy, int iz) const;

  // Linear index -> (ix,iy,iz)
  std::tuple<int,int,int> unravel(int idx) const;

private:
  int n_;
  AABB box_;
  double sx_, sy_, sz_;
  static int clampi(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
  }
};

} // namespace octoweave
