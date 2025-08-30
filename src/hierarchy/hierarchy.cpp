#include "octoweave/hierarchy.hpp"
#include "octoweave/union.hpp"
#include <array>
#include <cmath>
#include <functional>

namespace octoweave {

static inline Key3 parentKey(Key3 kc){ return { kc.x>>1, kc.y>>1, kc.z>>1 }; }
static inline int  childIndex(Key3 kc){ return (kc.x&1) | ((kc.y&1)<<1) | ((kc.z&1)<<2); }
static inline Key3 childKey (Key3 kp,int i){ return { (kp.x<<1)|((i>>0)&1), (kp.y<<1)|((i>>1)&1), (kp.z<<1)|((i>>2)&1) }; }

Hierarchy make_hierarchy_from_workers(const std::vector<WorkerOut>& outs,
                                      double tau, bool use_logodds,
                                      double p_unknown, int base_depth)
{
  // 1) Collect max td and union-merge all per-chunk maps into global P[td]
  int td = 0;
  for (auto& o : outs) td = std::max(td, o.td);

  // P[d][Key3] = prob
  std::unordered_map<int, std::unordered_map<Key3,double,Key3Hash>> P;
  auto& Ptd = P[td];

  for (auto& o : outs) {
    for (auto& kv : o.Ptd) {
      auto it = Ptd.find(kv.first);
      if (it == Ptd.end()) {
        Ptd.emplace(kv.first, std::clamp(kv.second, 0.0, 1.0));
      } else {
        double s = std::clamp(it->second, 0.0, 1.0);
        double p = std::clamp(kv.second, 0.0, 1.0);
        it->second = 1.0 - (1.0 - s) * (1.0 - p);
      }
    }
  }

  // 2) Roll up: td-1 ... base_depth
  for (int d = td-1; d >= base_depth; --d) {
    auto &Pc = P[d+1];
    auto &Pp = P[d];
    std::unordered_map<Key3, std::array<double,8>, Key3Hash> buckets;
    buckets.reserve(Pc.size()/4 + 8);

    for (auto& kv : Pc) {
      const Key3 kc = kv.first;
      const Key3 kp = parentKey(kc);
      auto &arr = buckets[kp];
      // default init (value-init leaves zeros); we will fill and then patch with p_unknown
      arr[childIndex(kc)] = kv.second;
    }

    for (auto& kv : buckets) {
      std::array<double,8> p8;
      for (int i=0;i<8;++i) {
        double v = kv.second[i];
        if (!(v >= 0.0 && v <= 1.0)) v = p_unknown;
        p8[i] = v;
      }
      Pp[kv.first] = union_prob8_stable(p8, p_unknown);
    }
  }

  // 3) Emit hierarchy with threshold and evidence guard
  auto passes = [&](double p)->bool{
    if (!use_logodds) return p >= tau;
    return std::log(p/(1.0-p)) >= tau;
  };
  auto has_child_evidence = [&](const Key3& k, int d)->bool{
    auto it = P.find(d+1);
    if (it == P.end()) return false;
    for (int i=0;i<8;++i) if (it->second.count(childKey(k,i))) return true;
    return false;
  };

  Hierarchy H; H.base_depth = base_depth; H.td = td;
  std::function<void(const Key3&,int)> emit = [&](const Key3& k, int d){
    double p = P[d].at(k);
    bool refine_ok = (d < td) && passes(p) && has_child_evidence(k, d);
    NDKey nd{ k, (uint16_t)d };
    if (!refine_ok) { H.nodes[nd] = NodeRec{ p, true }; return; }
    H.nodes[nd] = NodeRec{ p, false };
    for (int i=0;i<8;++i) {
      Key3 kc = childKey(k,i);
      auto it = P[d+1].find(kc);
      if (it != P[d+1].end()) emit(kc, d+1);
    }
  };

  for (auto& kv : P[base_depth]) emit(kv.first, base_depth);
  return H;
}

} // namespace octoweave
