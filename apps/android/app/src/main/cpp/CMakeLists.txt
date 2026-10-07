cmake_minimum_required(VERSION 3.22.1)

project(goreecloud_browser_android_recovery LANGUAGES CXX)

add_library(
  goreecloud_browser_android_recovery
  SHARED
  browser_normal_session_jni.cpp
)

target_compile_features(goreecloud_browser_android_recovery PRIVATE cxx_std_20)
target_compile_options(
  goreecloud_browser_android_recovery
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wconversion
  -Wsign-conversion
)

target_include_directories(
  goreecloud_browser_android_recovery
  PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../../../../..//include"
)
