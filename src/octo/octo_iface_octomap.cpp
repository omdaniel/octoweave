#ifdef OCTOWEAVE_WITH_OCTOMAP
#include "octoweave/octo_iface.hpp"
#include <octomap/OcTree.h>

namespace octoweave {

WorkerOut OctoChunker::build_and_export(const std::vector<Pt>& pts, const Params& p) {
  octomap::OcTree tree(p.res);
  tree.setProbHit(p.prob_hit);
  tree.setProbMiss(p.prob_miss);
  tree.setClampingThresMin(p.clamp_min);
  tree.setClampingThresMax(p.clamp_max);

  // Insert point cloud with free-space ray updates from the given origin
  octomap::Pointcloud cloud;
  cloud.reserve(pts.size());
  for (const auto& pt : pts) {
    cloud.push_back((float)pt.x, (float)pt.y, (float)pt.z);
  }
  octomap::point3d origin((float)p.origin.x, (float)p.origin.y, (float)p.origin.z);
  double maxrange = p.max_range > 0.0 ? p.max_range : -1.0;
  tree.insertPointCloud(cloud, origin, maxrange, p.lazy_eval, p.discretize);
  tree.updateInnerOccupancy();

  // Determine emission depth from desired resolution with a safety cap.
  const int td_tree = (int) tree.getTreeDepth();
  const double res_tree = tree.getResolution();
  int d_cap = p.max_depth_cap > 0 ? std::min(p.max_depth_cap, td_tree) : td_tree;
  int d_emit = d_cap;
  if (p.emit_res > 0.0) {
    double ratio = p.emit_res / res_tree;
    if (ratio > 1.0) {
      int shift = (int)std::floor(std::log2(ratio));
      d_emit = std::max(d_cap - shift, 0);
    }
  }

  WorkerOut out;
  out.td = d_emit;
  const int shift = td_tree - d_emit;

  // Export probabilities aggregated to the target emission depth
  for (auto it = tree.begin_leafs(); it != tree.end_leafs(); ++it) {
    octomap::OcTreeKey key;
    if (!tree.coordToKeyChecked(it.getCoordinate(), key)) continue;
    uint32_t kx = key.k[0];
    uint32_t ky = key.k[1];
    uint32_t kz = key.k[2];
    if (shift > 0) { kx >>= shift; ky >>= shift; kz >>= shift; }
    Key3 k{ kx, ky, kz };
    double prob = it->getOccupancy();
    double &slot = out.Ptd[k];
    slot = 1.0 - (1.0 - slot) * (1.0 - prob);
  }
  return out;
}

} // namespace octoweave

#endif // OCTOWEAVE_WITH_OCTOMAP
