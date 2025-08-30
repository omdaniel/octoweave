#include <catch2/catch_test_macros.hpp>
#include "octoweave/viz.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

using namespace octoweave;

TEST_CASE("octoweave_viz: slice + hist outputs") {
  namespace fs = std::filesystem;
  fs::create_directories("viz_tmp");
  std::string csv = "viz_tmp/in.csv";
  std::ofstream f(csv);
  f << "0,0,0,3,1.0\n";
  f << "1,0,0,3,0.5\n";
  f << "0,1,0,3,0.25\n";
  f << "2,0,1,3,0.75\n"; // different z slice
  f.close();

  std::string out = "viz_tmp/out.pgm";
  std::string svg = "viz_tmp/hist.svg";
  const char* argv[] = {"octoweave_viz", "--csv", csv.c_str(), "--slice_z", "0", "--depth", "3", "--out", out.c_str(), "--hist", svg.c_str()};
  int rc = viz_main(11, const_cast<char**>(argv));
  REQUIRE(rc == 0);

  // Load PGM and compute checksum of pixels
  std::ifstream pgm(out);
  REQUIRE((bool)pgm);
  std::string magic; int W,H, maxv; pgm >> magic >> W >> H >> maxv;
  REQUIRE(magic == "P2");
  REQUIRE(W == 2); REQUIRE(H == 2);
  std::vector<int> px(W*H); for (int i=0;i<W*H;++i) pgm >> px[i];
  int sum = 0; for (int v : px) sum += v;
  // Expected: (0,0)=255, (1,0)=128, (0,1)=64, (1,1)=0
  REQUIRE(sum == 255 + 128 + 64 + 0);

  // Check that SVG contains depth-count comment for d=3 with count 4
  std::ifstream os(svg);
  REQUIRE((bool)os);
  std::string content((std::istreambuf_iterator<char>(os)), {});
  REQUIRE(content.find("depth 3: count 4") != std::string::npos);
}

