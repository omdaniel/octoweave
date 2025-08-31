#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include "octoweave/chunk_grid.hpp"
#include "octoweave/octo_iface.hpp"
#include "octoweave/hierarchy.hpp"
#include "octoweave/viz.hpp"

int main(){
  using namespace octoweave;
  namespace fs = std::filesystem;
  std::cout << "[ex03] Generating CSV and rendering a slice...\n";

  // Build a tiny hierarchy (same pattern as example 1)
  ChunkGrid grid(2, AABB{0,4, 0,4, 0,4});
  std::vector<std::vector<Pt>> per_chunk(8);
  for (int iz=0; iz<2; ++iz) for (int iy=0; iy<2; ++iy) for (int ix=0; ix<2; ++ix) {
    auto B = grid.chunk_box(ix,iy,iz);
    per_chunk[ix + 2*(iy + 2*iz)] = {
      {B.xmin + 0.2, B.ymin + 0.2, B.zmin + 0.2},
      {B.xmin + 1.2, B.ymin + 0.2, B.zmin + 0.2}
    };
  }
  OctoChunker::Params p; p.res=0.25; p.emit_res=0.5; p.max_depth_cap=12;
  std::vector<WorkerOut> outs; outs.reserve(per_chunk.size());
  for (size_t i=0;i<per_chunk.size();++i) outs.push_back(OctoChunker::build_and_export(per_chunk[i], p));
  Hierarchy H = make_hierarchy_from_workers(outs, 0.5, false, 0.5, 1);

  fs::create_directories("examples_out");
  std::string csv = "examples_out/leaves_viz.csv";
  std::ofstream f(csv);
  int target_depth = H.td;
  for (auto& kv : H.nodes) if (kv.second.is_leaf) {
    auto k = kv.first.k; int d = kv.first.d; double pr = kv.second.p;
    f << k.x << "," << k.y << "," << k.z << "," << d << "," << pr << "\n";
  }
  std::cout << "[ex03] Wrote " << csv << "\n";

  // Render a 2D slice and an SVG histogram using the in-lib viz_main
  std::string pgm = "examples_out/slice.pgm";
  std::string svg = "examples_out/hist.svg";
  std::string sdepth = std::to_string(target_depth);
  const char* argv[] = {"octoweave_viz", "--csv", csv.c_str(), "--slice_z", "0", "--depth", sdepth.c_str(), "--out", pgm.c_str(), "--hist", svg.c_str()};
  int rc = viz_main(11, const_cast<char**>(argv));
  if (rc == 0) {
    std::cout << "[ex03] Wrote " << pgm << " and " << svg << "\n";
  } else {
    std::cout << "[ex03] viz_main returned " << rc << "\n";
  }
  return rc;
}

