#include <catch2/catch_test_macros.hpp>
#include "octoweave/union.hpp"
#include <array>

using namespace octoweave;

TEST_CASE("union_prob8_stable identities") {
  std::array<double,8> z{}; z.fill(0.0);
  std::array<double,8> o{}; o.fill(1.0);
  REQUIRE(union_prob8_stable(z) == Approx(0.0));
  REQUIRE(union_prob8_stable(o) == Approx(1.0));
}

TEST_CASE("union_prob8_stable symmetry and stability") {
  std::array<double,8> a{0.1, 0.2, 0.3, 0.05, 0.0, 0.9, 0.8, 0.4};
  auto b = a; std::reverse(b.begin(), b.end());
  REQUIRE(union_prob8_stable(a) == Approx(union_prob8_stable(b)));
  // Small probabilities add approximately linearly
  std::array<double,8> tiny{}; tiny.fill(1e-12);
  REQUIRE(union_prob8_stable(tiny) == Approx(8e-12).epsilon(1e-6));
  // Near-one probabilities remain numerically stable
  std::array<double,8> near1{}; near1.fill(1.0 - 1e-15);
  REQUIRE(union_prob8_stable(near1) == Approx(1.0).epsilon(1e-12));
  // Unknowns fall back to p_unknown
  std::array<double,8> mix{0.1, -5.0, std::nan(""), 0.2, 0.3, 0.0, 0.0, 0.0};
  double pu = 0.42;
  auto m = mix; m[1] = pu; m[2] = pu; // expected replacements
  REQUIRE(union_prob8_stable(mix, pu) == Approx(union_prob8_stable(m, pu)));
}
