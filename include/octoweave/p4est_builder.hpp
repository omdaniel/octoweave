#pragma once
#include "hierarchy.hpp"
#include <functional>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace octoweave {

// Note: Named P4estBuilder for consistency with the p4est project packaging.
// This implements a 3D octree mapping API (n^3 brick roots) with a stubbed
// backend by default. Real integration, if enabled, should be guarded by
// OCTOWEAVE_WITH_P4EST and include the appropriate headers from the p4est project
// (using the p8est API for 3D under the hood).
struct P4estBuilder {
  struct Config {
    int n = 2; // brick n×n×n
    // Optional per-tree target level policy. If unset, defaults to (H.td - H.base_depth).
    // The function receives the flattened tree index (0..n^3-1) and the hierarchy.
    std::function<int(int /*tree_idx*/, const Hierarchy&)> level_policy;
    int min_level = 0;
    int max_level = 30;
  };
  
  struct Policy {
    // Uniform level across all trees
    static inline std::function<int(int,const Hierarchy&)> uniform(int level) {
      return [level](int, const Hierarchy&) { return level; };
    }

    // Build a policy from a precomputed levels vector (size n^3)
    static inline std::function<int(int,const Hierarchy&)> from_levels(std::vector<int> levels) {
      // Capture by shared_ptr to keep small lambda and avoid copies
      auto sp = std::make_shared<std::vector<int>>(std::move(levels));
      return [sp](int tree_idx, const Hierarchy&) {
        if (tree_idx < 0 || (size_t)tree_idx >= sp->size()) return 0;
        return (*sp)[(size_t)tree_idx];
      };
    }

    // Compute per-tree levels by linearly mapping leaf counts to [Lmin, Lmax]
    static inline std::function<int(int,const Hierarchy&)> by_leafcount_linear(
        const Hierarchy& H, int n, int Lmin, int Lmax)
    {
      const size_t T = (size_t)n*n*n;
      std::vector<size_t> counts(T, 0);
      for (const auto& kv : H.nodes) {
        const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
        if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
        auto split = P4estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
        const Key3& t = split.first;
        size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
        if (idx < T) counts[idx] += 1;
      }
      size_t cmin = SIZE_MAX, cmax = 0;
      for (auto c : counts) { cmin = std::min(cmin, c); cmax = std::max(cmax, c); }
      std::vector<int> levels(T, Lmin);
      if (cmax == cmin) {
        std::fill(levels.begin(), levels.end(), Lmin);
      } else {
        for (size_t i=0;i<T;++i) {
          double t = (counts[i] - (double)cmin) / (double)(cmax - cmin);
          int L = (int)std::lround(Lmin + t * (double)(Lmax - Lmin));
          levels[i] = std::clamp(L, Lmin, Lmax);
        }
      }
      return from_levels(std::move(levels));
    }

    // Compute per-tree levels by mean probability threshold
    static inline std::function<int(int,const Hierarchy&)> by_mean_prob_threshold(
        const Hierarchy& H, int n, double threshold, int Llow, int Lhigh)
    {
      const size_t T = (size_t)n*n*n;
      std::vector<double> sum(T, 0.0); std::vector<size_t> cnt(T, 0);
      for (const auto& kv : H.nodes) {
        const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
        if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
        auto split = P4estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
        const Key3& t = split.first;
        size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
        if (idx < T) { sum[idx] += rec.p; cnt[idx] += 1; }
      }
      std::vector<int> levels(T, Llow);
      for (size_t i=0;i<T;++i) {
        double mean = cnt[i] ? (sum[i] / (double)cnt[i]) : 0.0;
        levels[i] = (mean >= threshold) ? Lhigh : Llow;
      }
      return from_levels(std::move(levels));
    }

    // Quantile-based mapping by leaf count: levels for [<=q_lo, between, >=q_hi]
    static inline std::function<int(int,const Hierarchy&)> by_leafcount_quantiles(
        const Hierarchy& H, int n, double q_lo, double q_hi, int Llow, int Lmid, int Lhigh)
    {
      const size_t T = (size_t)n*n*n;
      std::vector<size_t> counts(T, 0);
      for (const auto& kv : H.nodes) {
        const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
        if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
        auto split = P4estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
        const Key3& t = split.first;
        size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
        if (idx < T) counts[idx] += 1;
      }
      // Build sorted list and compute quantiles
      std::vector<size_t> sorted = counts;
      std::sort(sorted.begin(), sorted.end());
      auto qidx = [&](double q){
        if (sorted.empty()) return (size_t)0;
        double pos = q * (sorted.size() - 1);
        size_t i = (size_t) std::round(pos);
        if (i >= sorted.size()) i = sorted.size()-1;
        return i;
      };
      size_t thr_lo = sorted[qidx(std::clamp(q_lo, 0.0, 1.0))];
      size_t thr_hi = sorted[qidx(std::clamp(q_hi, 0.0, 1.0))];

      std::vector<int> levels(T, Lmid);
      for (size_t i=0;i<T;++i) {
        if (counts[i] <= thr_lo) levels[i] = Llow;
        else if (counts[i] >= thr_hi) levels[i] = Lhigh;
        else levels[i] = Lmid;
      }
      return from_levels(std::move(levels));
    }

    // Multi-threshold bands by count: thresholds ascending, levels.size()==thresholds.size()+1
    static inline std::function<int(int,const Hierarchy&)> bands_by_count(
        const Hierarchy& H, int n, const std::vector<size_t>& thresholds, const std::vector<int>& levels)
    {
      const size_t T = (size_t)n*n*n;
      if (levels.size() != thresholds.size() + 1) {
        // Fallback: uniform zero
        return uniform(0);
      }
      std::vector<size_t> counts(T, 0);
      for (const auto& kv : H.nodes) {
        const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
        if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
        auto split = P4estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
        const Key3& t = split.first;
        size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
        if (idx < T) counts[idx] += 1;
      }
      std::vector<int> out(T, levels.back());
      for (size_t i=0;i<T;++i) {
        size_t c = counts[i];
        size_t b = 0;
        while (b < thresholds.size() && c > thresholds[b]) ++b;
        if (b >= levels.size()) b = levels.size()-1;
        out[i] = levels[b];
      }
      return from_levels(std::move(out));
    }

    // Multi-threshold bands by mean probability (double thresholds ascending)
    static inline std::function<int(int,const Hierarchy&)> bands_by_mean_prob(
        const Hierarchy& H, int n, const std::vector<double>& thresholds, const std::vector<int>& levels)
    {
      const size_t T = (size_t)n*n*n;
      if (levels.size() != thresholds.size() + 1) {
        return uniform(0);
      }
      std::vector<double> sum(T, 0.0); std::vector<size_t> cnt(T, 0);
      for (const auto& kv : H.nodes) {
        const NDKey& nd = kv.first; const NodeRec& rec = kv.second;
        if (!rec.is_leaf || nd.d != (uint16_t)H.td) continue;
        auto split = P4estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
        const Key3& t = split.first;
        size_t idx = (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
        if (idx < T) { sum[idx] += rec.p; cnt[idx] += 1; }
      }
      std::vector<int> out(T, levels.back());
      for (size_t i=0;i<T;++i) {
        double m = cnt[i] ? (sum[i] / (double)cnt[i]) : 0.0;
        size_t b = 0;
        while (b < thresholds.size() && m > thresholds[b]) ++b;
        if (b >= levels.size()) b = levels.size()-1;
        out[i] = levels[b];
      }
      return from_levels(std::move(out));
    }
  };
  // Stub: map hierarchy to per-tree "want sets" structure (no external dep yet)
  static void prepare_want_sets(const Hierarchy& H, const Config& cfg);

  // Build a (stubbed or real) forest from the hierarchy and config. Returns 0 on success.
  static int build_forest(const Hierarchy& H, const Config& cfg);

  // Opaque forest handle for later phases (owns forest resources when real).
  struct ForestHandle {
    ~ForestHandle();
    void* impl = nullptr; // internal impl; nullptr in stub builds
  };
  // Create a forest handle (real under flag, opaque/stub otherwise).
  static ForestHandle* build_forest_handle(const Hierarchy& H, const Config& cfg);

  // Utility: split a global node key at depth d into (tree_idx, local_key)
  // Contract (stub): brick partitioning by modulo along each axis
  // - tree_idx = (k.x % n, k.y % n, k.z % n)
  // - local_key = (k.x / n, k.y / n, k.z / n)
  static std::pair<Key3, Key3> split_global_to_tree_local(const Key3& k, int d, int n);
};

} // namespace octoweave
