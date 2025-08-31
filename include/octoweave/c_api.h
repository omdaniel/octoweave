#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handles
typedef struct ow_hierarchy_s* ow_hierarchy_t;
typedef struct ow_forest_s* ow_forest_t;

// OctoChunker params for C API
typedef struct {
  double res;
  double prob_hit;
  double prob_miss;
  double clamp_min;
  double clamp_max;
  double origin_xyz[3];
  double max_range;   // <=0 unlimited
  int    lazy_eval;   // bool
  int    discretize;  // bool
  double emit_res;    // <=0 to use tree res
  int    max_depth_cap;
} ow_chunk_params_t;

// Build hierarchy from a flat array of XYZ triplets (size = 3*count)
ow_hierarchy_t ow_build_hierarchy_from_points(const double* xyz, size_t count,
                                              const ow_chunk_params_t* params,
                                              double tau,
                                              double p_unknown,
                                              int base_depth);

// Write hierarchy leaves to CSV (x,y,z,depth,prob)
int ow_hierarchy_write_csv(ow_hierarchy_t h, const char* path);

// Destroy hierarchy handle
void ow_hierarchy_free(ow_hierarchy_t h);

// Build a p8est forest with uniform target level (real build) or stub (returns success)
ow_forest_t ow_build_forest_uniform(ow_hierarchy_t h, int n, int level);

// Destroy forest handle
void ow_forest_free(ow_forest_t f);

// Compute per-tree levels by leafcount quantiles; out_levels must have length n^3
int ow_levels_by_leafcount_quantiles(ow_hierarchy_t h, int n,
                                     double q_lo, double q_hi,
                                     int Llow, int Lmid, int Lhigh,
                                     int* out_levels, size_t out_len);

// Compute per-tree levels by mean-probability bands; thresholds ascending, levels.size = thresholds.size+1
int ow_levels_bands_by_mean_prob(ow_hierarchy_t h, int n,
                                 const double* thresholds, size_t tlen,
                                 const int* levels, size_t llen,
                                 int* out_levels, size_t out_len);

// Visualization: wrapper for viz_main; null svg path to skip histogram
int ow_viz_slice(const char* csv_path, int slice_z, int depth,
                 const char* out_pgm, const char* out_svg);

#ifdef __cplusplus
}
#endif
