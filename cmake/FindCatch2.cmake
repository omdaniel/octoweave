# Minimal offline stub for Catch2 providing Catch2::Catch2WithMain

set(_ow_catch2_root "${CMAKE_CURRENT_LIST_DIR}/../third_party/catch2")
if(EXISTS "${_ow_catch2_root}/catch_test_macros.hpp")
  add_library(Catch2WithMain STATIC
    "${_ow_catch2_root}/stub_main.cpp"
  )
  # Provide include root that contains the `catch2/` folder
  get_filename_component(_ow_catch2_inc_root "${_ow_catch2_root}/.." ABSOLUTE)
  target_include_directories(Catch2WithMain PUBLIC "${_ow_catch2_inc_root}")
  add_library(Catch2::Catch2WithMain ALIAS Catch2WithMain)
  set(Catch2_FOUND TRUE)
else()
  set(Catch2_FOUND FALSE)
endif()
