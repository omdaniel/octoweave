#pragma once
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace catch2_stub {

struct TestRegistry {
  using Fn = void(*)();
  std::vector<std::pair<std::string, Fn>> tests;
  static TestRegistry& inst() { static TestRegistry R; return R; }
  void add(const std::string& name, Fn fn) { tests.emplace_back(name, fn); }
};

struct Approx {
  double value;
  double eps = 1e-12;
  explicit Approx(double v): value(v) {}
  Approx& epsilon(double e){ eps = e; return *this; }
};

inline bool approx_eq(double a, const Approx& b){ return std::fabs(a - b.value) <= b.eps * (1 + std::fabs(b.value)); }

} // namespace catch2_stub

#define CATCH_INTERNAL_CONCAT_IMPL(x,y) x##y
#define CATCH_INTERNAL_CONCAT(x,y) CATCH_INTERNAL_CONCAT_IMPL(x,y)

#define TEST_CASE(Name) \
  static void CATCH_INTERNAL_CONCAT(test_fn_, __LINE__)(); \
  static struct CATCH_INTERNAL_CONCAT(test_reg_, __LINE__) { \
    CATCH_INTERNAL_CONCAT(test_reg_, __LINE__)(){ catch2_stub::TestRegistry::inst().add((Name), &CATCH_INTERNAL_CONCAT(test_fn_, __LINE__)); } \
  } CATCH_INTERNAL_CONCAT(test_reg_inst_, __LINE__); \
  static void CATCH_INTERNAL_CONCAT(test_fn_, __LINE__)()

#define REQUIRE(cond) do { \
  if(!(cond)) { \
    std::cerr << "REQUIRE failed: " #cond " at " << __FILE__ << ":" << __LINE__ << "\n"; \
    std::exit(1); \
  } \
} while(0)

// Overloads to support REQUIRE(a == Approx(b)) style
inline bool operator==(double a, const catch2_stub::Approx& b){ return catch2_stub::approx_eq(a, b); }
inline bool operator==(const catch2_stub::Approx& a, double b){ return catch2_stub::approx_eq(b, a); }

// Support `REQUIRE(x == Approx(y))` when Approx is on RHS or LHS
#define Approx catch2_stub::Approx
