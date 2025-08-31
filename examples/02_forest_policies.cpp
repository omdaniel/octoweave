#include <iostream>
#include <vector>
#include "octoweave/chunk_grid.hpp"
#include "octoweave/octo_iface.hpp"
#include "octoweave/hierarchy.hpp"
#include "octoweave/p8est_builder.hpp"

static octoweave::Hierarchy build_small_hierarchy() {
  using namespace octoweave;
  ChunkGrid grid(2, AABB{0,4, 0,4, 0,4});
  std::vector<std::vector<Pt>> per_chunk(8);
  for (int iz=0; iz<2; ++iz) for (int iy=0; iy<2; ++iy) for (int ix=0; ix<2; ++ix) {
    auto B = grid.chunk_box(ix,iy,iz);
    per_chunk[ix + 2*(iy + 2*iz)] = {
      {B.xmin + 0.2, B.ymin + 0.2, B.zmin + 0.2},
      {B.xmin + 1.2, B.ymin + 0.2, B.zmin + 0.2}
    };
  }
  octoweave::OctoChunker::Params p; p.res=0.25; p.emit_res=0.5; p.max_depth_cap=12;
  std::vector<octoweave::WorkerOut> outs; outs.reserve(per_chunk.size());
  for (size_t i=0;i<per_chunk.size();++i) outs.push_back(OctoChunker::build_and_export(per_chunk[i], p));
  return make_hierarchy_from_workers(outs, 0.5, false, 0.5, 1);
}

int main(){
  using namespace octoweave;
  std::cout << "[ex02] Building forest with different policies...\n";
  Hierarchy H = build_small_hierarchy();
  P8estBuilder::Config cfg; cfg.n = 2; cfg.min_level = 0; cfg.max_level = 12;

#ifdef OCTOWEAVE_WITH_P8EST
  // 1) Uniform policy
  cfg.level_policy = P8estBuilder::Policy::uniform(6);
  std::cout << "[ex02] Uniform policy L=6\n";
  P8estBuilder::build_forest(H, cfg);

  // 2) Linear by leaf count between L=4..10
  cfg.level_policy = P8estBuilder::Policy::by_leafcount_linear(H, cfg.n, 4, 10);
  std::cout << "[ex02] by_leafcount_linear L in [4,10]\n";
  P8estBuilder::build_forest(H, cfg);

  // 3) Quantile bands by count: 20% (low=4), middle (7), 80% (high=10)
  cfg.level_policy = P8estBuilder::Policy::by_leafcount_quantiles(H, cfg.n, 0.2, 0.8, 4, 7, 10);
  std::cout << "[ex02] by_leafcount_quantiles q_lo=0.2 q_hi=0.8\n";
  P8estBuilder::build_forest(H, cfg);

  // 4) Bands by mean probability
  cfg.level_policy = P8estBuilder::Policy::bands_by_mean_prob(H, cfg.n, {0.3, 0.6, 0.85}, {4, 7, 9, 11});
  std::cout << "[ex02] bands_by_mean_prob thresholds 0.3/0.6/0.85\n";
  P8estBuilder::build_forest(H, cfg);

  // 5) Opaque forest handle with a given policy
  cfg.level_policy = P8estBuilder::Policy::by_mean_prob_threshold(H, cfg.n, 0.5, 6, 9);
  auto* fh = P8estBuilder::build_forest_handle(H, cfg);
  std::cout << "[ex02] forest handle created (and will be destroyed)\n";
  delete fh;
#else
  std::cout << "[ex02] OCTOWEAVE_WITH_P8EST is OFF; skipping real forest build.\n";
#endif
  return 0;
}

