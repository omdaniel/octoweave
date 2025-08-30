cmake_minimum_required(VERSION 3.16)
project(OctoWeave LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(OCTOWEAVE_WITH_OCTOMAP "Enable OctoMap integration" OFF)
option(OCTOWEAVE_WITH_P8EST   "Enable p8est integration"   OFF)
option(OCTOWEAVE_BUILD_TESTS  "Build unit tests"           ON)

# Allow using a local Catch2 stub when offline
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Catch2 QUIET)
if(NOT Catch2_FOUND)
  include(FetchContent)
  # Catch2 (header-only v3)
  FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
  )
  FetchContent_MakeAvailable(catch2)
endif()

add_library(octoweave STATIC
  src/chunk/chunk_grid.cpp
  src/union/prob_union.cpp
  src/hierarchy/hierarchy.cpp
  src/utils/logging.cpp
  src/octo/octo_iface_stub.cpp
  src/p8est/p8est_builder_stub.cpp
  src/parallel/parallel.cpp
)
target_include_directories(octoweave PUBLIC include)

add_executable(octoweave_viz
  src/viz/viz_main.cpp
  src/viz/viz_impl.cpp
)
target_link_libraries(octoweave_viz PRIVATE octoweave)

if (OCTOWEAVE_BUILD_TESTS)
  enable_testing()
  add_executable(ow_unit_tests
    tests/unit/test_chunk_grid.cpp
    tests/unit/test_prob_union.cpp
    tests/unit/test_hierarchy.cpp
    tests/unit/test_octo_iface.cpp
    tests/unit/test_p8est_mapping.cpp
  )
  target_link_libraries(ow_unit_tests PRIVATE octoweave Catch2::Catch2WithMain)
  include(CTest)
  add_test(NAME ow_unit COMMAND ow_unit_tests)
endif()
