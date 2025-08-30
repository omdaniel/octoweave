#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace octoweave {

struct Key3 { uint32_t x, y, z; bool operator==(const Key3& o) const noexcept {
  return x==o.x && y==o.y && z==o.z; }};

struct Key3Hash {
  size_t operator()(Key3 const& k) const noexcept {
    uint64_t h = (uint64_t)k.x * 0x9e3779b185ebca87ULL;
    h ^= ((uint64_t)k.y + 0x9e3779b185ebca87ULL + (h<<6) + (h>>2));
    h ^= ((uint64_t)k.z + 0x9e3779b185ebca87ULL + (h<<6) + (h>>2));
    return (size_t)h;
  }
};

struct NDKey { Key3 k; uint16_t d;
  bool operator==(const NDKey& o) const noexcept { return d==o.d && k==o.k; }};
struct NDHash {
  size_t operator()(NDKey const& a) const noexcept {
    return Key3Hash{}(a.k) ^ (size_t)a.d * 0x9e3779b1; }
};

struct NodeRec { double p; bool is_leaf; };

struct Hierarchy {
  std::unordered_map<NDKey, NodeRec, NDHash> nodes;
  int base_depth = 1;
  int td = 1;
};

struct WorkerOut {
  // probability map at max depth td
  std::unordered_map<Key3,double,Key3Hash> Ptd;
  int td = 0;
};

/// Build a hierarchy from per-chunk WorkerOut results.
/// - tau: probability threshold (if comparing in log-odds, pass `use_logodds=true`)
/// - p_unknown: default probability for missing children
Hierarchy make_hierarchy_from_workers(
  const std::vector<WorkerOut>& outs,
  double tau, bool use_logodds=false,
  double p_unknown=0.5, int base_depth=1);

} // namespace octoweave
