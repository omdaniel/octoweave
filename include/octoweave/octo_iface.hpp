#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include "hierarchy.hpp"

namespace octoweave {

struct Pt { double x, y, z; };

class IOctoTree {
public:
  virtual ~IOctoTree() = default;
  virtual int max_depth() const = 0;
  virtual std::vector<std::tuple<Key3,double>> export_leaves_at_depth(int td) const = 0; // (key, prob)
};

/// Stubbed chunk builder; concrete OctoMap version will implement this later.
class OctoChunker {
public:
  struct Params {
    double res = 0.05;
    double prob_hit  = 0.7;
    double prob_miss = 0.4;
    double clamp_min = 0.12;
    double clamp_max = 0.97;
  };
  // Build a per-chunk tree from points and export WorkerOut
  static WorkerOut build_and_export(const std::vector<Pt>& pts, const Params& p);
};

// Build an in-memory stub tree from a WorkerOut for testing/integration.
std::unique_ptr<IOctoTree> make_stub_tree_from_worker(const WorkerOut& w);

} // namespace octoweave
