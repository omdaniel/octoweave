#pragma once
#include "hierarchy.hpp"

namespace octoweave {

struct P8estBuilder {
  struct Config {
    int n = 2; // brick n×n×n
  };
  // Stub: map hierarchy to per-tree "want sets" structure (no p8est dep yet)
  static void prepare_want_sets(const Hierarchy& H, const Config& cfg);

  // Utility: split a global node key at depth d into (tree_idx, local_key)
  // Contract (stub): brick partitioning by modulo along each axis
  // - tree_idx = (k.x % n, k.y % n, k.z % n)
  // - local_key = (k.x / n, k.y / n, k.z / n)
  static std::pair<Key3, Key3> split_global_to_tree_local(const Key3& k, int d, int n);
};

} // namespace octoweave
