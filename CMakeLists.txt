cmake_minimum_required(VERSION 3.16)
project(OctoWeave LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(OCTOWEAVE_WITH_OCTOMAP "Enable OctoMap integration" OFF)
option(OCTOWEAVE_WITH_P8EST   "Enable p8est integration"   OFF)
option(OCTOWEAVE_BUILD_TESTS  "Build unit tests"           ON)
option(OCTOWEAVE_BUILD_EXAMPLES "Build example programs"   ON)
option(OCTOWEAVE_BUILD_PYTHON   "Prepare Python ctypes lib" ON)
option(OCTOWEAVE_BUILD_DOCS     "Add docs target if sphinx-build is found" ON)

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

# Optional external integrations (OctoMap / p8est via MacPorts/Homebrew)
find_package(PkgConfig QUIET)

add_library(octoweave STATIC
  src/chunk/chunk_grid.cpp
  src/union/prob_union.cpp
  src/hierarchy/hierarchy.cpp
  src/utils/logging.cpp
  src/octo/octo_iface_stub.cpp
  src/octo/octo_iface_octomap.cpp
  src/p8est/p8est_builder_stub.cpp
  src/p8est/p8est_builder_real.cpp
  src/parallel/parallel.cpp
  src/viz/viz_impl.cpp
)
target_include_directories(octoweave PUBLIC include)

add_executable(octoweave_viz
  src/viz/viz_main.cpp
)
target_link_libraries(octoweave_viz PRIVATE octoweave)

# --- OctoMap integration (headers+libs via Homebrew paths or pkg-config) ---
if (OCTOWEAVE_WITH_OCTOMAP)
  message(STATUS "OCTOWEAVE_WITH_OCTOMAP=ON: looking for OctoMap")
  # Try package config first
  if (PkgConfig_FOUND)
    pkg_check_modules(OCTOMAP QUIET octomap)
  endif()
  # Hints for typical Homebrew install
  if (NOT OCTOMAP_FOUND)
    find_path(OCTOMAP_INCLUDE_DIR octomap/OcTree.h
      HINTS
        $ENV{OCTOMAP_ROOT}/include
        ${CMAKE_PREFIX_PATH}/include
        /opt/homebrew/opt/octomap/include
        /usr/local/opt/octomap/include
    )
    find_library(OCTOMAP_LIB octomap
      HINTS
        $ENV{OCTOMAP_ROOT}/lib $ENV{OCTOMAP_ROOT}/lib64
        ${CMAKE_PREFIX_PATH}/lib ${CMAKE_PREFIX_PATH}/lib64
        /opt/homebrew/opt/octomap/lib
        /usr/local/opt/octomap/lib
    )
    find_library(OCTOMATH_LIB octomath
      HINTS
        $ENV{OCTOMAP_ROOT}/lib $ENV{OCTOMAP_ROOT}/lib64
        ${CMAKE_PREFIX_PATH}/lib ${CMAKE_PREFIX_PATH}/lib64
        /opt/homebrew/opt/octomap/lib
        /usr/local/opt/octomap/lib
    )
    if (OCTOMAP_INCLUDE_DIR AND OCTOMAP_LIB AND OCTOMATH_LIB)
      set(OCTOMAP_FOUND TRUE)
      set(OCTOMAP_INCLUDE_DIRS ${OCTOMAP_INCLUDE_DIR})
      set(OCTOMAP_LIBRARIES ${OCTOMAP_LIB} ${OCTOMATH_LIB})
      # Common lib dirs for rpath
      get_filename_component(_ow_octomap_libdir "${OCTOMAP_LIB}" DIRECTORY)
    endif()
  else()
    # pkg-config provided variables
    set(OCTOMAP_INCLUDE_DIRS ${OCTOMAP_INCLUDE_DIRS})
    set(OCTOMAP_LIBRARIES ${OCTOMAP_LINK_LIBRARIES})
  endif()

  if (OCTOMAP_FOUND)
    message(STATUS "Found OctoMap: includes='${OCTOMAP_INCLUDE_DIRS}', libs='${OCTOMAP_LIBRARIES}'")
    target_compile_definitions(octoweave PUBLIC OCTOWEAVE_WITH_OCTOMAP=1)
    target_include_directories(octoweave PUBLIC ${OCTOMAP_INCLUDE_DIRS})
    target_link_libraries(octoweave PUBLIC ${OCTOMAP_LIBRARIES})
    # RPATH hints (prefer rpath to DYLD_LIBRARY_PATH)
    if (APPLE)
      # Prefer precise directories if known from pkg-config or find_library
      if (DEFINED OCTOMAP_LIBRARY_DIRS)
        list(APPEND _ow_octomap_rpaths ${OCTOMAP_LIBRARY_DIRS})
      endif()
      if (DEFINED _ow_octomap_libdir)
        list(APPEND _ow_octomap_rpaths ${_ow_octomap_libdir})
      endif()
      list(APPEND _ow_octomap_rpaths 
        /opt/homebrew/opt/octomap/lib
        /usr/local/opt/octomap/lib)
      list(REMOVE_DUPLICATES _ow_octomap_rpaths)
      set_target_properties(octoweave PROPERTIES BUILD_RPATH "${_ow_octomap_rpaths}")
      if (TARGET octoweave_viz)
        set_target_properties(octoweave_viz PROPERTIES BUILD_RPATH "${_ow_octomap_rpaths}")
      endif()
      if (TARGET ow_unit_tests)
        set_target_properties(ow_unit_tests PROPERTIES BUILD_RPATH "${_ow_octomap_rpaths}")
      endif()
    endif()
    # Compile a tiny probe TU to ensure headers/libs are exercised
    target_sources(octoweave PRIVATE src/octo/octomap_probe.cpp)
  else()
    message(FATAL_ERROR "OCTOWEAVE_WITH_OCTOMAP=ON but OctoMap not found. Set CMAKE_PREFIX_PATH or OCTOMAP_ROOT.")
  endif()
endif()

# Docs target (optional)
if (OCTOWEAVE_BUILD_DOCS)
  find_program(SPHINX_EXECUTABLE sphinx-build)
  if (SPHINX_EXECUTABLE)
    add_custom_target(docs
      COMMAND ${SPHINX_EXECUTABLE} -b html
              ${CMAKE_SOURCE_DIR}/docs
              ${CMAKE_SOURCE_DIR}/docs/_build/html
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building Sphinx documentation")
  else()
    message(STATUS "sphinx-build not found; 'docs' target will be unavailable")
  endif()
endif()

# --- p8est integration (via MacPorts pkg-config) ---
if (OCTOWEAVE_WITH_P8EST)
  message(STATUS "OCTOWEAVE_WITH_P8EST=ON: looking for p8est (and sc/p4est)")
  set(_ow_have_p8est FALSE)
  if (PkgConfig_FOUND)
    pkg_check_modules(P8EST QUIET p8est)
    if (P8EST_FOUND)
      set(_ow_have_p8est TRUE)
      message(STATUS "Found p8est via pkg-config: cflags='${P8EST_CFLAGS}', libs='${P8EST_LIBRARIES}', libdirs='${P8EST_LIBRARY_DIRS}'")
      target_include_directories(octoweave PUBLIC ${P8EST_INCLUDE_DIRS})
      target_link_directories(octoweave PUBLIC ${P8EST_LIBRARY_DIRS})
      target_link_libraries(octoweave PUBLIC ${P8EST_LIBRARIES})
    endif()
  endif()

  # Fallback: manual search at common MacPorts locations
  if (NOT _ow_have_p8est)
    find_path(P8EST_INCLUDE_DIR p8est.h
      HINTS /opt/local/include ${CMAKE_PREFIX_PATH}/include)
    find_library(P8EST_LIB p8est HINTS /opt/local/lib ${CMAKE_PREFIX_PATH}/lib)
    find_library(P4EST_LIB p4est HINTS /opt/local/lib ${CMAKE_PREFIX_PATH}/lib)
    find_library(SC_LIB sc HINTS /opt/local/lib ${CMAKE_PREFIX_PATH}/lib)
    if (P8EST_INCLUDE_DIR AND P8EST_LIB AND P4EST_LIB AND SC_LIB)
      set(_ow_have_p8est TRUE)
      set(P8EST_INCLUDE_DIRS ${P8EST_INCLUDE_DIR})
      set(P8EST_LIBRARY_DIRS /opt/local/lib ${CMAKE_PREFIX_PATH}/lib)
      set(P8EST_LIBRARIES ${P8EST_LIB} ${P4EST_LIB} ${SC_LIB})
      message(STATUS "Found p8est via manual search: includes='${P8EST_INCLUDE_DIRS}', libs='${P8EST_LIBRARIES}'")
      target_include_directories(octoweave PUBLIC ${P8EST_INCLUDE_DIRS})
      target_link_libraries(octoweave PUBLIC ${P8EST_LIBRARIES})
    endif()
  endif()

  if (NOT _ow_have_p8est)
    message(FATAL_ERROR "OCTOWEAVE_WITH_P8EST=ON but p8est not found. Ensure pkg-config can see p8est or set CMAKE_PREFIX_PATH (e.g., /opt/local).")
  endif()

  target_compile_definitions(octoweave PUBLIC OCTOWEAVE_WITH_P8EST=1)
  if (APPLE)
    set(_ow_p8_rpaths ${P8EST_LIBRARY_DIRS} /opt/local/lib)
    list(REMOVE_DUPLICATES _ow_p8_rpaths)
    # Append to any existing rpaths from OctoMap
    get_target_property(_ow_existing_rpath octoweave BUILD_RPATH)
    if (_ow_existing_rpath)
      list(APPEND _ow_p8_rpaths ${_ow_existing_rpath})
    endif()
    set_target_properties(octoweave PROPERTIES BUILD_RPATH "${_ow_p8_rpaths}")
    if (TARGET octoweave_viz)
      set_target_properties(octoweave_viz PROPERTIES BUILD_RPATH "${_ow_p8_rpaths}")
    endif()
    if (TARGET ow_unit_tests)
      set_target_properties(ow_unit_tests PROPERTIES BUILD_RPATH "${_ow_p8_rpaths}")
    endif()
  endif()
  # Tiny probe TU ensures link references p8est symbols
  target_sources(octoweave PRIVATE src/p8est/p8est_probe.cpp)
endif()

if (OCTOWEAVE_BUILD_TESTS)
  enable_testing()
  add_executable(ow_unit_tests
    tests/unit/test_chunk_grid.cpp
    tests/unit/test_prob_union.cpp
    tests/unit/test_hierarchy.cpp
    tests/unit/test_octo_iface.cpp
    tests/unit/test_p8est_mapping.cpp
    tests/unit/test_parallel.cpp
    tests/unit/test_viz.cpp
    tests/unit/test_end_to_end.cpp
  )
  target_link_libraries(ow_unit_tests PRIVATE octoweave Catch2::Catch2WithMain)
  include(CTest)
  add_test(NAME ow_unit COMMAND ow_unit_tests)
endif()

# Examples
if (OCTOWEAVE_BUILD_EXAMPLES)
  add_executable(ex01_synthetic_hierarchy examples/01_synthetic_hierarchy.cpp)
  target_link_libraries(ex01_synthetic_hierarchy PRIVATE octoweave)

  add_executable(ex02_forest_policies examples/02_forest_policies.cpp)
  target_link_libraries(ex02_forest_policies PRIVATE octoweave)

  add_executable(ex03_viz_export examples/03_viz_export.cpp)
  target_link_libraries(ex03_viz_export PRIVATE octoweave)

  add_executable(ex04_csv_to_forest examples/04_csv_to_forest.cpp)
  target_link_libraries(ex04_csv_to_forest PRIVATE octoweave)

  add_executable(ex05_parallel_chunks examples/05_parallel_chunks.cpp)
  target_link_libraries(ex05_parallel_chunks PRIVATE octoweave)
endif()

# Python ctypes shared library (no external deps)
if (OCTOWEAVE_BUILD_PYTHON)
  add_library(octoweave_c SHARED src/c_api.cpp)
  target_include_directories(octoweave_c PUBLIC include)
  target_link_libraries(octoweave_c PRIVATE octoweave)
  set_target_properties(octoweave_c PROPERTIES OUTPUT_NAME "octoweave_c")
  # Copy the shared lib next to the Python package for simple packaging
  add_custom_command(TARGET octoweave_c POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/python/octoweave_py"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:octoweave_c> "${CMAKE_CURRENT_SOURCE_DIR}/python/octoweave_py/"
  )
  # Install rule for scikit-build-core to place the library inside the Python package
  install(TARGETS octoweave_c
    RUNTIME DESTINATION octoweave_py
    LIBRARY DESTINATION octoweave_py
    ARCHIVE DESTINATION octoweave_py)
endif()
