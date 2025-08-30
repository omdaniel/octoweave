#include "octoweave/viz.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <limits>
#include <cmath>

namespace octoweave {

namespace {
struct Rec { int x,y,z,d; double p; };

static bool parse_csv(const std::string& path, std::vector<Rec>& out) {
  std::ifstream f(path);
  if (!f) return false;
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty()) continue;
    std::istringstream ss(line);
    std::string tok;
    Rec r{};
    if (!std::getline(ss, tok, ',')) return false; r.x = std::stoi(tok);
    if (!std::getline(ss, tok, ',')) return false; r.y = std::stoi(tok);
    if (!std::getline(ss, tok, ',')) return false; r.z = std::stoi(tok);
    if (!std::getline(ss, tok, ',')) return false; r.d = std::stoi(tok);
    if (!std::getline(ss, tok, ',')) return false; r.p = std::stod(tok);
    out.push_back(r);
  }
  return true;
}

static bool write_pgm(const std::string& path, int W, int H, const std::vector<int>& img) {
  std::ofstream o(path);
  if (!o) return false;
  o << "P2\n" << W << " " << H << "\n255\n";
  for (int y=0;y<H;++y){
    for (int x=0;x<W;++x){ o << img[y*W + x]; if (x+1<W) o << ' '; }
    o << '\n';
  }
  return true;
}

static bool write_hist_svg(const std::string& path, const std::unordered_map<int,int>& counts) {
  int W = 400, H = 200, pad = 20;
  int max_count = 1;
  for (auto& kv : counts) if (kv.second > max_count) max_count = kv.second;
  std::ofstream o(path);
  if (!o) return false;
  o << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<W<<"\" height=\""<<H<<"\">\n";
  o << "<rect x=\"0\" y=\"0\" width=\""<<W<<"\" height=\""<<H<<"\" fill=\"white\"/>\n";
  int n = (int)counts.size(); if (n==0) n=1; double barW = (W - 2*pad) / (double)n;
  int i=0; for (auto& kv : counts) {
    int d = kv.first, c = kv.second;
    double h = (H - 2*pad) * (c / (double)max_count);
    double x = pad + i*barW; double y = H - pad - h;
    o << "<rect x=\""<<x<<"\" y=\""<<y<<"\" width=\""<<barW-2<<"\" height=\""<<h<<"\" fill=\"#4a90e2\"/>\n";
    o << "<!-- depth "<<d<<": count "<<c<<" -->\n";
    ++i;
  }
  o << "</svg>\n";
  return true;
}
}

int viz_main(int argc, char** argv) {
  // Parse args
  std::string csv, out_img, out_svg; int slice_z = 0; int depth = std::numeric_limits<int>::min();
  for (int i=1;i<argc;++i) {
    std::string a = argv[i];
    auto need = [&](int& i){ return (i+1<argc) ? i+1 : i; };
    if (a == "--csv" && i+1<argc) { csv = argv[++i]; }
    else if (a == "--out" && i+1<argc) { out_img = argv[++i]; }
    else if (a == "--hist" && i+1<argc) { out_svg = argv[++i]; }
    else if (a == "--slice_z" && i+1<argc) { slice_z = std::stoi(argv[++i]); }
    else if (a == "--depth" && i+1<argc) { depth = std::stoi(argv[++i]); }
  }
  if (csv.empty() || out_img.empty()) {
    std::fprintf(stderr, "Usage: octoweave_viz --csv file.csv --slice_z k --out out.pgm [--depth d] [--hist out.svg]\n");
    return 2;
  }

  std::vector<Rec> recs; if (!parse_csv(csv, recs)) return 3;
  // Determine depth if not provided (use max)
  if (depth == std::numeric_limits<int>::min()) {
    depth = 0; for (auto& r : recs) depth = std::max(depth, r.d);
  }

  // Build slice image
  int xmax=0, ymax=0; bool any=false;
  for (auto& r : recs) if (r.d==depth && r.z==slice_z) { xmax = std::max(xmax, r.x); ymax = std::max(ymax, r.y); any=true; }
  int W = xmax+1, H = ymax+1;
  if (!any) { W = H = 1; }
  std::vector<int> img(W*H, 0);
  std::unordered_map<int,int> counts;
  for (auto& r : recs) {
    counts[r.d] += 1;
    if (r.d!=depth || r.z!=slice_z) continue;
    int val = (int)std::lround(std::max(0.0, std::min(1.0, r.p)) * 255.0);
    int idx = r.y*W + r.x; if (idx >= 0 && idx < (int)img.size()) img[idx] = val;
  }
  if (!write_pgm(out_img, W, H, img)) return 4;
  if (!out_svg.empty()) { if (!write_hist_svg(out_svg, counts)) return 5; }

  return 0;
}

} // namespace octoweave

