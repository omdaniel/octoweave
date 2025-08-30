#include "catch_test_macros.hpp"
#include <iostream>

int main() {
  auto& R = catch2_stub::TestRegistry::inst();
  int failed = 0;
  for (auto& t : R.tests) {
    try {
      t.second();
    } catch (const std::exception& e) {
      std::cerr << "Test threw exception: " << t.first << ": " << e.what() << "\n";
      ++failed;
    } catch (...) {
      std::cerr << "Test threw unknown exception: " << t.first << "\n";
      ++failed;
    }
  }
  if (failed) {
    std::cerr << failed << " test(s) failed\n";
    return 1;
  }
  std::cout << R.tests.size() << " test(s) passed\n";
  return 0;
}

