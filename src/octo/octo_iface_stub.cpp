#include "octoweave/octo_iface.hpp"
#include <unordered_map>

namespace octoweave {

class StubTree : public IOctoTree {
public:
  int td_;
  std::vector<std::tuple<Key3,double>> leaves_;
  explicit StubTree(int td): td_(td) {}
  int max_depth() const override { return td_; }
  std::vector<std::tuple<Key3,double>> export_leaves_at_depth(int td) const override {
    (void)td; return leaves_;
  }
};

#ifndef OCTOWEAVE_WITH_OCTOMAP
WorkerOut OctoChunker::build_and_export(const std::vector<Pt>& pts, const Params& p) {
  // Stub: place points in a trivial grid cell and accumulate with a simple union
  (void) p;
  WorkerOut out; out.td = p.max_depth_cap > 0 ? p.max_depth_cap : 8;
  for (auto& pt : pts) {
    Key3 k{ (uint32_t)(pt.x), (uint32_t)(pt.y), (uint32_t)(pt.z) };
    double &slot = out.Ptd[k];
    double p1 = 0.7; // pretend-hit
    slot = 1.0 - (1.0 - slot) * (1.0 - p1);
  }
  return out;
}
#endif

std::unique_ptr<IOctoTree> make_stub_tree_from_worker(const WorkerOut& w) {
  auto t = std::make_unique<StubTree>(w.td);
  t->leaves_.reserve(w.Ptd.size());
  for (auto& kv : w.Ptd) t->leaves_.emplace_back(kv.first, kv.second);
  return t;
}

} // namespace octoweave
