cmake_minimum_required(VERSION 3.22.1)

project(goreecloud_browser_android_session LANGUAGES CXX)

add_library(
  goreecloud_browser_android_session
  SHARED
  normal_session_recovery_jni.cpp
)

target_compile_features(goreecloud_browser_android_session PRIVATE cxx_std_20)
target_compile_options(
  goreecloud_browser_android_session
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(
  goreecloud_browser_android_session
  PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../../../../..//include"
)
