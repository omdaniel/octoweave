#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include "octoweave/hierarchy.hpp"
#include "octoweave/p8est_builder.hpp"

// Parse CSV lines: x,y,z,depth,prob
static bool parse_csv(const std::string& path, std::vector<std::tuple<octoweave::Key3,int,double>>& out) {
  std::ifstream f(path);
  if (!f) return false;
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty()) continue;
    std::istringstream ss(line);
    std::string tok;
    octoweave::Key3 k{}; int d=0; double p=0.0;
    if (!std::getline(ss, tok, ',')) return false; k.x = (uint32_t)std::stoul(tok);
    if (!std::getline(ss, tok, ',')) return false; k.y = (uint32_t)std::stoul(tok);
    if (!std::getline(ss, tok, ',')) return false; k.z = (uint32_t)std::stoul(tok);
    if (!std::getline(ss, tok, ',')) return false; d = std::stoi(tok);
    if (!std::getline(ss, tok, ',')) return false; p = std::stod(tok);
    out.emplace_back(k, d, p);
  }
  return true;
}

int main(int argc, char** argv) {
  using namespace octoweave;
  std::string csv = argc > 1 ? argv[1] : "examples_out/leaves.csv";
  std::cout << "[ex04] Reading CSV: " << csv << "\n";
  std::vector<std::tuple<Key3,int,double>> recs;
  if (!parse_csv(csv, recs)) { std::cerr << "Failed to read CSV\n"; return 2; }

  // Build a minimal Hierarchy using CSV leaves
  Hierarchy H; H.base_depth = 1; H.td = 0;
  for (auto& r : recs) {
    Key3 k; int d; double p; std::tie(k,d,p) = r;
    H.td = std::max(H.td, d);
    NDKey nd{ k, (uint16_t)d };
    auto it = H.nodes.find(nd);
    if (it == H.nodes.end()) {
      H.nodes.emplace(nd, NodeRec{ p, true });
    } else {
      double& slot = it->second.p;
      slot = 1.0 - (1.0 - slot) * (1.0 - p);
      it->second.is_leaf = true;
    }
  }
  std::cout << "[ex04] H: td=" << H.td << ", leaf_count=";
  size_t lc=0; for (auto& kv : H.nodes) if (kv.second.is_leaf) ++lc; std::cout << lc << "\n";

  P8estBuilder::Config cfg; cfg.n = 2; cfg.min_level = 0; cfg.max_level = 12;
  cfg.level_policy = P8estBuilder::Policy::uniform(std::max(0, H.td - H.base_depth));

#ifdef OCTOWEAVE_WITH_P8EST
  std::cout << "[ex04] Building p8est forest...\n";
  int rc = P8estBuilder::build_forest(H, cfg);
  std::cout << "[ex04] build_forest rc=" << rc << "\n";
#else
  std::cout << "[ex04] OCTOWEAVE_WITH_P8EST is OFF; printing want sets only.\n";
  P8estBuilder::prepare_want_sets(H, cfg);
#endif
  return 0;
}

