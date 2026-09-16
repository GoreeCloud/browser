cmake_minimum_required(VERSION 3.24)

project(goreecloud_browser_extension_signature_verify
  VERSION 0.1.0
  DESCRIPTION "Standalone verification harness for GoreeCloud Browser GCEX signatures"
  LANGUAGES CXX
)

include(CTest)

get_filename_component(GOREECLOUD_BROWSER_SOURCE_DIR
  "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

find_path(SODIUM_INCLUDE_DIR sodium.h REQUIRED)
find_library(SODIUM_LIBRARY NAMES sodium libsodium REQUIRED)

add_library(goreecloud_browser_extension_signature STATIC
  "${GOREECLOUD_BROWSER_SOURCE_DIR}/src/extension_signature_sodium.cpp"
)
target_include_directories(goreecloud_browser_extension_signature
  PUBLIC
    "${GOREECLOUD_BROWSER_SOURCE_DIR}/include"
  PRIVATE
    "${SODIUM_INCLUDE_DIR}"
)
target_compile_features(goreecloud_browser_extension_signature PUBLIC cxx_std_20)
target_link_libraries(goreecloud_browser_extension_signature PUBLIC "${SODIUM_LIBRARY}")

if(MSVC)
  target_compile_options(goreecloud_browser_extension_signature PRIVATE /W4 /permissive- /WX)
else()
  target_compile_options(goreecloud_browser_extension_signature PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(goreecloud_browser_extension_signature_smoke
  "${GOREECLOUD_BROWSER_SOURCE_DIR}/apps/browser/extension_signature_smoke.cpp"
)
target_include_directories(goreecloud_browser_extension_signature_smoke
  PRIVATE "${SODIUM_INCLUDE_DIR}")
target_compile_features(goreecloud_browser_extension_signature_smoke PRIVATE cxx_std_20)
target_link_libraries(goreecloud_browser_extension_signature_smoke
  PRIVATE goreecloud_browser_extension_signature)

if(MSVC)
  target_compile_options(goreecloud_browser_extension_signature_smoke PRIVATE /W4 /permissive- /WX)
else()
  target_compile_options(goreecloud_browser_extension_signature_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
  add_test(NAME goreecloud_browser_extension_signature_smoke
           COMMAND goreecloud_browser_extension_signature_smoke)
endif()
