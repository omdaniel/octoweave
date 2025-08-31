#ifdef OCTOWEAVE_WITH_P8EST
#include "octoweave/p8est_builder.hpp"
extern "C" {
#include <p8est.h>
#include <p8est_connectivity.h>
#include <p8est_iterate.h>
}
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace octoweave {

void P8estBuilder::prepare_want_sets(const Hierarchy& H, const Config& cfg) {
  // Touch p8est to validate link; create and destroy a connectivity object.
  p8est_connectivity_t *conn = p8est_connectivity_new_brick(cfg.n, cfg.n, cfg.n, 1, 0);
  p8est_connectivity_destroy(conn);

  size_t leaves = 0, internals = 0;
  for (auto& kv : H.nodes) {
    if (kv.second.is_leaf) ++leaves; else ++internals;
  }
  std::printf("[P8estBuilder(real)] n=%d, nodes=%zu (leaves=%zu, internals=%zu)\n",
              cfg.n, H.nodes.size(), leaves, internals);
}

std::pair<Key3, Key3> P8estBuilder::split_global_to_tree_local(const Key3& k, int /*d*/, int n) {
  Key3 tree{ (uint32_t)(k.x % (uint32_t)n), (uint32_t)(k.y % (uint32_t)n), (uint32_t)(k.z % (uint32_t)n) };
  Key3 local{ (uint32_t)(k.x / (uint32_t)n), (uint32_t)(k.y / (uint32_t)n), (uint32_t)(k.z / (uint32_t)n) };
  return {tree, local};
}

// Internal implementation storage for the opaque handle
namespace {
  struct ForestImpl {
    p8est_connectivity_t* conn = nullptr;
    p8est_t* forest = nullptr;
  };

  struct QuadKey {
    int tree;
    uint32_t x, y, z;
    bool operator==(const QuadKey& o) const noexcept {
      return tree==o.tree && x==o.x && y==o.y && z==o.z;
    }
  };
  struct QuadKeyHash {
    size_t operator()(QuadKey const& k) const noexcept {
      uint64_t h = (uint64_t) (k.tree + 0x9e37);
      h ^= ((uint64_t)k.x * 0x9e3779b185ebca87ULL) + (h<<6) + (h>>2);
      h ^= ((uint64_t)k.y * 0x94d049bb133111ebULL) + (h<<6) + (h>>2);
      h ^= ((uint64_t)k.z * 0xda942042e4dd58b5ULL) + (h<<6) + (h>>2);
      return (size_t)h;
    }
  };
}

static void init_quadrant_prob(p8est_t* p8, p8est_topidx_t which_tree, p8est_quadrant_t* q) {
  struct Ctx { int Ltarget; const std::vector<char>* refine; const std::vector<double>* means; };
  Ctx* c = static_cast<Ctx*>(p8->user_pointer);
  if (!c || !c->means) return;
  if (!q) return;
  double* d = (double*) q->p.user_data;
  if (d) {
    size_t idx = (size_t) which_tree;
    double val = (idx < c->means->size()) ? (*(c->means))[idx] : 0.0;
    *d = val;
  }
}

int P8estBuilder::build_forest(const Hierarchy& H, const Config& cfg) {
  // Deterministic policy: For each tree that contains at least one leaf at H.td,
  // refine uniformly to level Ltarget = max(0, H.td - H.base_depth). Others remain coarse.
  int n = cfg.n;
  int Ltarget = H.td - H.base_depth;
  if (Ltarget < 0) Ltarget = 0;

  std::vector<char> tree_has_content((size_t)n*n*n, 0);
  for (const auto& kv : H.nodes) {
    const NDKey& nd = kv.first;
    const NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
    auto split = P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    const Key3& t = split.first;
    size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
    if (idx < tree_has_content.size()) tree_has_content[idx] = 1;
  }

  // Compute per-tree mean probability from hierarchy leaves at H.td
  std::vector<double> tree_means((size_t)n*n*n, 0.0);
  std::vector<size_t> tree_counts((size_t)n*n*n, 0);
  for (const auto& kv : H.nodes) {
    const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
    auto split = P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    const Key3& t = split.first;
    size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
    if (idx < tree_means.size()) { tree_means[idx] += rec.p; tree_counts[idx] += 1; }
  }
  for (size_t i=0;i<tree_means.size();++i) if (tree_counts[i]) tree_means[i] /= (double) tree_counts[i];

  p8est_connectivity_t *conn = p8est_connectivity_new_brick(n, n, n, 1, 0);
  if (!conn) return 1;

  // Compute per-tree target level via policy hook or uniform default
  std::vector<int> tree_levels((size_t)n*n*n, Ltarget);
  if (cfg.level_policy) {
    for (size_t ti = 0; ti < tree_levels.size(); ++ti) {
      int lvl = cfg.level_policy((int)ti, H);
      if (lvl < cfg.min_level) lvl = cfg.min_level;
      if (lvl > cfg.max_level) lvl = cfg.max_level;
      tree_levels[ti] = lvl;
    }
  }

  struct RefineCtx { const std::vector<char>* refine; const std::vector<int>* levels; } rctx{ &tree_has_content, &tree_levels };

  // Create a forest with per-quadrant data: one double (probability)
  sc_MPI_Comm mpicomm = sc_MPI_COMM_SELF;
  p8est_t *p8 = p8est_new_ext(mpicomm, conn, /*min_quadrants*/ 0, /*min_level*/ 0,
                              /*fill_uniform*/ 0, /*data_size*/ (int)sizeof(double),
                              /*init_fn*/ NULL, /*user_pointer*/ &rctx);
  if (!p8) { p8est_connectivity_destroy(conn); return 2; }

  auto refine_cb = [](p8est_t* p8est, p8est_topidx_t which_tree, p8est_quadrant_t* q) -> int {
    RefineCtx* c = static_cast<RefineCtx*>(p8est->user_pointer);
    if (!c || !c->refine || !c->levels) return 0;
    size_t idx = (size_t) which_tree;
    if (idx >= c->refine->size() || idx >= c->levels->size()) return 0;
    if (!(*(c->refine))[idx]) return 0;
    int target = (*(c->levels))[idx];
    return q->level < target ? 1 : 0;
  };

  // No per-quadrant data; init callback unused.
  p8est_refine(p8, 1, refine_cb, NULL);
  p8est_balance(p8, P8EST_CONNECT_FULL, NULL);

  // Build per-quadrant means at the tree-specific target levels
  std::unordered_map<QuadKey, double, QuadKeyHash> sum;
  std::unordered_map<QuadKey, uint32_t, QuadKeyHash> cnt;
  int n = cfg.n;
  for (const auto& kv : H.nodes) {
    const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
    auto split = P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    const Key3& t = split.first; const Key3& local = split.second;
    size_t tidx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
    int Lt = (tidx < tree_levels.size()) ? tree_levels[tidx] : Ltarget;
    int shift = (int)nd.d - Lt; if (shift < 0) shift = 0;
    QuadKey qk{ (int)tidx, local.x >> shift, local.y >> shift, local.z >> shift };
    sum[qk] += rec.p; cnt[qk] += 1;
  }
  std::unordered_map<QuadKey, double, QuadKeyHash> mean;
  mean.reserve(sum.size());
  for (auto& kv : sum) {
    mean.emplace(kv.first, kv.second / (double) cnt[kv.first]);
  }

  // Iterate all leaves and set their user data from the mean map
  struct IterCtx { const std::vector<int>* levels; decltype(mean)* pmean; } ictx{ &tree_levels, &mean };
  auto volume_cb = [](p8est_iter_volume_info_t* info, void* u) {
    IterCtx* ic = static_cast<IterCtx*>(u);
    int tree = (int) info->treeid;
    int Lt = 0;
    if (ic->levels && (size_t)tree < ic->levels->size()) Lt = (*(ic->levels))[tree];
    int len = P8EST_QUADRANT_LEN(Lt);
    uint32_t cx = (uint32_t) (info->quad->x / len);
    uint32_t cy = (uint32_t) (info->quad->y / len);
    uint32_t cz = (uint32_t) (info->quad->z / len);
    QuadKey qk{ tree, cx, cy, cz };
    auto it = ic->pmean->find(qk);
    double val = (it == ic->pmean->end()) ? 0.0 : it->second;
    double* d = (double*) info->quad->p.user_data;
    if (d) *d = val;
  };
  p8est_iterate(p8, NULL, &ictx, volume_cb, NULL, NULL, NULL);

  p8est_destroy(p8);
  p8est_connectivity_destroy(conn);
  return 0;
}

P8estBuilder::ForestHandle::~ForestHandle() {
  if (!impl) return;
  ForestImpl* f = reinterpret_cast<ForestImpl*>(impl);
  if (f->forest) p8est_destroy(f->forest);
  if (f->conn) p8est_connectivity_destroy(f->conn);
  delete f;
  impl = nullptr;
}

P8estBuilder::ForestHandle* P8estBuilder::build_forest_handle(const Hierarchy& H, const Config& cfg) {
  int n = cfg.n;
  int Ltarget = H.td - H.base_depth;
  if (Ltarget < 0) Ltarget = 0;
  std::vector<char> tree_has_content((size_t)n*n*n, 0);
  std::vector<double> tree_means((size_t)n*n*n, 0.0);
  std::vector<size_t> tree_counts((size_t)n*n*n, 0);
  for (const auto& kv : H.nodes) {
    const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
    auto split = P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    const Key3& t = split.first;
    size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
    if (idx < tree_has_content.size()) tree_has_content[idx] = 1;
    if (idx < tree_means.size()) { tree_means[idx] += rec.p; tree_counts[idx] += 1; }
  }
  for (size_t i=0;i<tree_means.size();++i) if (tree_counts[i]) tree_means[i] /= (double) tree_counts[i];

  p8est_connectivity_t *conn = p8est_connectivity_new_brick(n, n, n, 1, 0);
  if (!conn) return nullptr;

  // Compute per-tree target levels via policy hook or uniform default
  std::vector<int> tree_levels((size_t)n*n*n, Ltarget);
  if (cfg.level_policy) {
    for (size_t ti = 0; ti < tree_levels.size(); ++ti) {
      int lvl = cfg.level_policy((int)ti, H);
      if (lvl < cfg.min_level) lvl = cfg.min_level;
      if (lvl > cfg.max_level) lvl = cfg.max_level;
      tree_levels[ti] = lvl;
    }
  }

  struct RefineCtx { const std::vector<char>* refine; const std::vector<int>* levels; } rctx{ &tree_has_content, &tree_levels };

  sc_MPI_Comm mpicomm = sc_MPI_COMM_SELF;
  p8est_t *p8 = p8est_new_ext(mpicomm, conn, 0, 0, 0, (int)sizeof(double), NULL, &rctx);
  if (!p8) { p8est_connectivity_destroy(conn); return nullptr; }

  auto refine_cb = [](p8est_t* p8est, p8est_topidx_t which_tree, p8est_quadrant_t* q) -> int {
    struct RefineCtx { const std::vector<char>* refine; const std::vector<int>* levels; };
    RefineCtx* c = static_cast<RefineCtx*>(p8est->user_pointer);
    if (!c || !c->refine || !c->levels) return 0;
    size_t idx = (size_t) which_tree;
    if (idx >= c->refine->size() || idx >= c->levels->size()) return 0;
    if (!(*(c->refine))[idx]) return 0;
    int target = (*(c->levels))[idx];
    return q->level < target ? 1 : 0;
  };
  p8est_refine(p8, 1, refine_cb, NULL);
  p8est_balance(p8, P8EST_CONNECT_FULL, NULL);

  // Build per-quadrant means at tree-specific levels
  std::unordered_map<QuadKey, double, QuadKeyHash> sum;
  std::unordered_map<QuadKey, uint32_t, QuadKeyHash> cnt;
  for (const auto& kv : H.nodes) {
    const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
    auto split = P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    const Key3& t = split.first; const Key3& local = split.second;
    size_t tidx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
    int Lt = (tidx < tree_levels.size()) ? tree_levels[tidx] : Ltarget;
    int shift = (int)nd.d - Lt; if (shift < 0) shift = 0;
    QuadKey qk{ (int)tidx, local.x >> shift, local.y >> shift, local.z >> shift };
    sum[qk] += rec.p; cnt[qk] += 1;
  }
  std::unordered_map<QuadKey, double, QuadKeyHash> mean;
  mean.reserve(sum.size());
  for (auto& kv : sum) mean.emplace(kv.first, kv.second / (double) cnt[kv.first]);

  struct IterCtx { const std::vector<int>* levels; decltype(mean)* pmean; } ictx{ &tree_levels, &mean };
  auto volume_cb = [](p8est_iter_volume_info_t* info, void* u) {
    IterCtx* ic = static_cast<IterCtx*>(u);
    int tree = (int) info->treeid;
    int Lt = 0;
    if (ic->levels && (size_t)tree < ic->levels->size()) Lt = (*(ic->levels))[tree];
    int len = P8EST_QUADRANT_LEN(Lt);
    uint32_t cx = (uint32_t) (info->quad->x / len);
    uint32_t cy = (uint32_t) (info->quad->y / len);
    uint32_t cz = (uint32_t) (info->quad->z / len);
    QuadKey qk{ tree, cx, cy, cz };
    auto it = ic->pmean->find(qk);
    double val = (it == ic->pmean->end()) ? 0.0 : it->second;
    double* d = (double*) info->quad->p.user_data;
    if (d) *d = val;
  };
  p8est_iterate(p8, NULL, &ictx, volume_cb, NULL, NULL, NULL);

  ForestImpl* impl = new ForestImpl();
  impl->conn = conn;
  impl->forest = p8;
  auto* handle = new P8estBuilder::ForestHandle();
  handle->impl = impl;
  return handle;
}

} // namespace octoweave

#endif // OCTOWEAVE_WITH_P8EST
