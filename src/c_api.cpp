#include "octoweave/c_api.h"
#include "octoweave/octo_iface.hpp"
#include "octoweave/hierarchy.hpp"
#include "octoweave/p8est_builder.hpp"
#include "octoweave/viz.hpp"
#include <vector>
#include <fstream>
#include <algorithm>

struct ow_hierarchy_s { octoweave::Hierarchy H; };
struct ow_forest_s { void* impl; /* reserved */ };

extern "C" {

ow_hierarchy_t ow_build_hierarchy_from_points(const double* xyz, size_t count,
                                              const ow_chunk_params_t* params,
                                              double tau,
                                              double p_unknown,
                                              int base_depth)
{
  if (!xyz || !params) return nullptr;
  std::vector<octoweave::Pt> pts; pts.reserve(count);
  for (size_t i=0;i<count;++i) {
    pts.push_back(octoweave::Pt{ xyz[3*i+0], xyz[3*i+1], xyz[3*i+2] });
  }
  octoweave::OctoChunker::Params p;
  p.res = params->res;
  p.prob_hit = params->prob_hit;
  p.prob_miss = params->prob_miss;
  p.clamp_min = params->clamp_min;
  p.clamp_max = params->clamp_max;
  p.origin = octoweave::Pt{ params->origin_xyz[0], params->origin_xyz[1], params->origin_xyz[2] };
  p.max_range = params->max_range;
  p.lazy_eval = params->lazy_eval != 0;
  p.discretize = params->discretize != 0;
  p.emit_res = params->emit_res;
  p.max_depth_cap = params->max_depth_cap;

  octoweave::WorkerOut w = octoweave::OctoChunker::build_and_export(pts, p);
  std::vector<octoweave::WorkerOut> outs; outs.push_back(std::move(w));
  octoweave::Hierarchy H = octoweave::make_hierarchy_from_workers(outs, tau, /*use_logodds=*/false, p_unknown, base_depth);
  auto* h = new ow_hierarchy_s(); h->H = std::move(H);
  return h;
}

int ow_hierarchy_write_csv(ow_hierarchy_t h, const char* path) {
  if (!h || !path) return 1;
  std::ofstream f(path);
  if (!f) return 2;
  for (auto& kv : h->H.nodes) if (kv.second.is_leaf) {
    auto k = kv.first.k; int d = kv.first.d; double p = kv.second.p;
    f << k.x << "," << k.y << "," << k.z << "," << d << "," << p << "\n";
  }
  return 0;
}

void ow_hierarchy_free(ow_hierarchy_t h) {
  delete h;
}

ow_forest_t ow_build_forest_uniform(ow_hierarchy_t h, int n, int level) {
  if (!h || n <= 0) return nullptr;
  octoweave::P8estBuilder::Config cfg; cfg.n = n; cfg.min_level = 0; cfg.max_level = 30;
  cfg.level_policy = octoweave::P8estBuilder::Policy::uniform(level);
#ifdef OCTOWEAVE_WITH_P8EST
  auto* fh = octoweave::P8estBuilder::build_forest_handle(h->H, cfg);
  auto* f = new ow_forest_s(); f->impl = (void*) fh; return f;
#else
  octoweave::P8estBuilder::prepare_want_sets(h->H, cfg);
  auto* f = new ow_forest_s(); f->impl = nullptr; return f;
#endif
}

void ow_forest_free(ow_forest_t f) {
#ifdef OCTOWEAVE_WITH_P8EST
  if (f && f->impl) {
    auto* fh = reinterpret_cast<octoweave::P8estBuilder::ForestHandle*>(f->impl);
    delete fh; f->impl = nullptr;
  }
#endif
  delete f;
}

static inline size_t flat_tree_index(const octoweave::Key3& t, int n) {
  return (size_t)t.x + (size_t)n * ((size_t)t.y + (size_t)n * (size_t)t.z);
}

int ow_levels_by_leafcount_quantiles(ow_hierarchy_t h, int n,
                                     double q_lo, double q_hi,
                                     int Llow, int Lmid, int Lhigh,
                                     int* out_levels, size_t out_len)
{
  if (!h || n <= 0 || !out_levels) return 1;
  const size_t T = (size_t)n*n*n;
  if (out_len < T) return 2;
  std::vector<size_t> counts(T, 0);
  for (const auto& kv : h->H.nodes) {
    const octoweave::NDKey& nd = kv.first; const octoweave::NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)h->H.td) continue;
    auto split = octoweave::P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    size_t idx = flat_tree_index(split.first, n);
    if (idx < T) counts[idx] += 1;
  }
  std::vector<size_t> sorted = counts;
  std::sort(sorted.begin(), sorted.end());
  auto qidx = [&](double q){ if (sorted.empty()) return (size_t)0; double pos = std::clamp(q,0.0,1.0) * (sorted.size()-1); size_t i=(size_t)std::round(pos); if (i>=sorted.size()) i=sorted.size()-1; return i; };
  size_t thr_lo = sorted[qidx(q_lo)];
  size_t thr_hi = sorted[qidx(q_hi)];
  for (size_t i=0;i<T;++i) {
    if (counts[i] <= thr_lo) out_levels[i] = Llow;
    else if (counts[i] >= thr_hi) out_levels[i] = Lhigh;
    else out_levels[i] = Lmid;
  }
  return 0;
}

int ow_levels_bands_by_mean_prob(ow_hierarchy_t h, int n,
                                 const double* thresholds, size_t tlen,
                                 const int* levels, size_t llen,
                                 int* out_levels, size_t out_len)
{
  if (!h || n <= 0 || !out_levels || !levels) return 1;
  if (llen != tlen + 1) return 2;
  const size_t T = (size_t)n*n*n;
  if (out_len < T) return 3;
  std::vector<double> sum(T, 0.0); std::vector<size_t> cnt(T, 0);
  for (const auto& kv : h->H.nodes) {
    const octoweave::NDKey& nd = kv.first; const octoweave::NodeRec& rec = kv.second;
    if (!rec.is_leaf || nd.d != (uint16_t)h->H.td) continue;
    auto split = octoweave::P8estBuilder::split_global_to_tree_local(nd.k, nd.d, n);
    size_t idx = flat_tree_index(split.first, n);
    if (idx < T) { sum[idx] += rec.p; cnt[idx] += 1; }
  }
  for (size_t i=0;i<T;++i) {
    double m = cnt[i] ? (sum[i] / (double)cnt[i]) : 0.0;
    size_t b = 0; while (b < tlen && m > thresholds[b]) ++b;
    if (b >= llen) b = llen-1;
    out_levels[i] = levels[b];
  }
  return 0;
}

int ow_viz_slice(const char* csv_path, int slice_z, int depth,
                 const char* out_pgm, const char* out_svg)
{
  if (!csv_path || !out_pgm) return 1;
  const char* argv[12]; int argc = 0;
  argv[argc++] = "octoweave_viz";
  argv[argc++] = "--csv"; argv[argc++] = csv_path;
  argv[argc++] = "--slice_z"; static char zbuf[32]; snprintf(zbuf,sizeof zbuf, "%d", slice_z); argv[argc++] = zbuf;
  if (depth >= 0) { argv[argc++] = "--depth"; static char dbuf[32]; snprintf(dbuf,sizeof dbuf, "%d", depth); argv[argc++] = dbuf; }
  argv[argc++] = "--out"; argv[argc++] = out_pgm;
  if (out_svg && *out_svg) { argv[argc++] = "--hist"; argv[argc++] = out_svg; }
  return octoweave::viz_main(argc, const_cast<char**>(argv));
}

} // extern "C"
