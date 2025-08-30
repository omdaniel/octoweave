#include "octoweave/chunk_grid.hpp"

namespace octoweave {

ChunkGrid::ChunkGrid(int n, AABB box) : n_(n), box_(box) {
  sx_ = (box_.xmax - box_.xmin) / n_;
  sy_ = (box_.ymax - box_.ymin) / n_;
  sz_ = (box_.zmax - box_.zmin) / n_;
}

std::tuple<int,int,int,int> ChunkGrid::which(double x, double y, double z) const {
  int ix = clampi(int((x - box_.xmin)/sx_), 0, n_-1);
  int iy = clampi(int((y - box_.ymin)/sy_), 0, n_-1);
  int iz = clampi(int((z - box_.zmin)/sz_), 0, n_-1);
  int idx = ix + n_ * (iy + n_ * iz);
  return {ix,iy,iz,idx};
}

AABB ChunkGrid::chunk_box(int ix, int iy, int iz) const {
  return {
    box_.xmin + ix*sx_, box_.xmin + (ix+1)*sx_,
    box_.ymin + iy*sy_, box_.ymin + (iy+1)*sy_,
    box_.zmin + iz*sz_, box_.zmin + (iz+1)*sz_
  };
}

std::tuple<int,int,int> ChunkGrid::unravel(int idx) const {
  int iz = idx / (n_*n_);
  int rem = idx % (n_*n_);
  int iy = rem / n_;
  int ix = rem % n_;
  return {ix,iy,iz};
}

} // namespace octoweave
