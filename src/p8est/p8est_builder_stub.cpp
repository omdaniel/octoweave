#include "octoweave/p8est_builder.hpp"
#include <cstdio>

namespace octoweave {

void P8estBuilder::prepare_want_sets(const Hierarchy& H, const Config& cfg) {
  // Stub: print a tiny summary so we know mapping will be called later.
  size_t leaves = 0, internals = 0;
  for (auto& kv : H.nodes) {
    if (kv.second.is_leaf) ++leaves; else ++internals;
  }
  std::printf("[P8estBuilder] n=%d, nodes=%zu (leaves=%zu, internals=%zu)\n",
              cfg.n, H.nodes.size(), leaves, internals);
}

std::pair<Key3, Key3> P8estBuilder::split_global_to_tree_local(const Key3& k, int /*d*/, int n) {
  Key3 tree{ (uint32_t)(k.x % (uint32_t)n), (uint32_t)(k.y % (uint32_t)n), (uint32_t)(k.z % (uint32_t)n) };
  Key3 local{ (uint32_t)(k.x / (uint32_t)n), (uint32_t)(k.y / (uint32_t)n), (uint32_t)(k.z / (uint32_t)n) };
  return {tree, local};
}

} // namespace octoweave
