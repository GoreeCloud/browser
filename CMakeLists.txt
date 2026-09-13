cmake_minimum_required(VERSION 3.24)

project(goreecloud_browser
  VERSION 0.1.0
  DESCRIPTION "GoreeCloud first-party native browser"
  LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)

option(GOREECLOUD_ENABLE_CHROMIUM
  "Build the GoreeCloud Chromium engine adapter integration target"
  OFF
)
option(GOREECLOUD_ENABLE_CEF
  "Build the CEF-backed Chromium runtime path"
  OFF
)
option(GOREECLOUD_ENABLE_LINUX_GTK_HOST
  "Build the GTK3/X11 GoreeCloud desktop beta host"
  OFF
)
option(GOREECLOUD_ENABLE_CURL_DOWNLOAD_TRANSPORT
  "Build the libcurl-backed Advanced Download Manager HTTP transport"
  OFF
)

set(GOREECLOUD_CEF_ROOT "" CACHE PATH
    "Path to an unpacked CEF binary distribution")
set(GOREECLOUD_RELEASE_CHANNEL "beta" CACHE STRING
    "GoreeCloud Browser release channel")

if(GOREECLOUD_ENABLE_CEF AND NOT GOREECLOUD_ENABLE_CHROMIUM)
  message(FATAL_ERROR "GOREECLOUD_ENABLE_CEF requires GOREECLOUD_ENABLE_CHROMIUM=ON")
endif()
if(GOREECLOUD_ENABLE_LINUX_GTK_HOST AND NOT UNIX)
  message(FATAL_ERROR "GOREECLOUD_ENABLE_LINUX_GTK_HOST is currently supported only on Linux/Unix")
endif()

set(GOREECLOUD_GLAZE_UI_STABLE_VERSION "1.4.0" CACHE STRING
    "Current approved Stable Glaze UI version required by GoreeCloud Browser")
set(GOREECLOUD_GLAZE_UI_STABLE_REVISION
    "84cb3db4884042f0fa25ed6d475a127fb110f596" CACHE STRING
    "Exact reviewed Glaze UI Stable source revision")

if(NOT GOREECLOUD_GLAZE_UI_STABLE_VERSION STREQUAL "1.4.0")
  message(FATAL_ERROR
    "GoreeCloud Browser source currently accepts GLAZE UI Stable 1.4.0. "
    "Update the Browser Glaze adoption contract and acceptance evidence before changing the build baseline.")
endif()

add_library(goreecloud_browser_core INTERFACE)
target_include_directories(goreecloud_browser_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(goreecloud_browser_core INTERFACE cxx_std_20)
target_compile_definitions(goreecloud_browser_core INTERFACE
  GOREECLOUD_GLAZE_UI_STABLE_VERSION="${GOREECLOUD_GLAZE_UI_STABLE_VERSION}"
  GOREECLOUD_GLAZE_UI_STABLE_REVISION="${GOREECLOUD_GLAZE_UI_STABLE_REVISION}"
  GOREECLOUD_RELEASE_CHANNEL="${GOREECLOUD_RELEASE_CHANNEL}"
)

function(goreecloud_apply_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

if(GOREECLOUD_ENABLE_CURL_DOWNLOAD_TRANSPORT)
  find_package(CURL REQUIRED)
  add_library(goreecloud_browser_download_curl STATIC
    src/curl_http_download_client.cpp
  )
  target_link_libraries(goreecloud_browser_download_curl
    PUBLIC goreecloud_browser_core CURL::libcurl)
  target_compile_definitions(goreecloud_browser_download_curl
    PUBLIC GOREECLOUD_ENABLE_CURL_DOWNLOAD_TRANSPORT=1)
  goreecloud_apply_warnings(goreecloud_browser_download_curl)
endif()

if(GOREECLOUD_ENABLE_CHROMIUM)
  add_library(goreecloud_browser_chromium STATIC
    src/chromium_adapter.cpp
    src/chromium_runtime_scaffold.cpp
  )
  target_link_libraries(goreecloud_browser_chromium PUBLIC goreecloud_browser_core)
  target_compile_definitions(goreecloud_browser_chromium PUBLIC GOREECLOUD_ENABLE_CHROMIUM=1)

  if(GOREECLOUD_ENABLE_CEF)
    if(GOREECLOUD_CEF_ROOT STREQUAL "")
      message(FATAL_ERROR "GOREECLOUD_ENABLE_CEF requires GOREECLOUD_CEF_ROOT")
    endif()
    if(NOT EXISTS "${GOREECLOUD_CEF_ROOT}/include/cef_app.h")
      message(FATAL_ERROR "GOREECLOUD_CEF_ROOT does not contain include/cef_app.h")
    endif()
    if(NOT EXISTS "${GOREECLOUD_CEF_ROOT}/cmake/FindCEF.cmake")
      message(FATAL_ERROR "GOREECLOUD_CEF_ROOT does not contain cmake/FindCEF.cmake")
    endif()

    set(CEF_ROOT "${GOREECLOUD_CEF_ROOT}")
    list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
    find_package(CEF REQUIRED)

    if(NOT TARGET libcef_lib)
      ADD_LOGICAL_TARGET(libcef_lib "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
    endif()
    if(NOT TARGET libcef_dll_wrapper)
      add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH}
                       ${CMAKE_BINARY_DIR}/libcef_dll_wrapper)
    endif()

    target_sources(goreecloud_browser_chromium PRIVATE src/cef_runtime.cpp)
    target_include_directories(goreecloud_browser_chromium PRIVATE ${CEF_INCLUDE_PATH})
    target_link_libraries(goreecloud_browser_chromium
      PUBLIC
        libcef_dll_wrapper
        libcef_lib
        ${CEF_STANDARD_LIBS}
    )
    target_compile_definitions(goreecloud_browser_chromium PUBLIC GOREECLOUD_ENABLE_CEF=1)
    SET_LIBRARY_TARGET_PROPERTIES(goreecloud_browser_chromium)
  else()
    target_compile_definitions(goreecloud_browser_chromium PUBLIC GOREECLOUD_ENABLE_CEF=0)
  endif()

  goreecloud_apply_warnings(goreecloud_browser_chromium)
endif()

add_executable(goreecloud_browser
  apps/browser/main.cpp
  src/cef_process.cpp
)
set_target_properties(goreecloud_browser PROPERTIES OUTPUT_NAME "goreecloud-browser")
target_link_libraries(goreecloud_browser PRIVATE goreecloud_browser_core)

if(GOREECLOUD_ENABLE_CHROMIUM)
  target_link_libraries(goreecloud_browser PRIVATE goreecloud_browser_chromium)
  target_compile_definitions(goreecloud_browser PRIVATE GOREECLOUD_ENABLE_CHROMIUM=1)
else()
  target_compile_definitions(goreecloud_browser PRIVATE GOREECLOUD_ENABLE_CHROMIUM=0 GOREECLOUD_ENABLE_CEF=0)
endif()

if(GOREECLOUD_ENABLE_CURL_DOWNLOAD_TRANSPORT)
  target_link_libraries(goreecloud_browser PRIVATE goreecloud_browser_download_curl)
endif()

if(GOREECLOUD_ENABLE_CEF)
  target_include_directories(goreecloud_browser PRIVATE ${CEF_INCLUDE_PATH})
  target_link_libraries(goreecloud_browser PRIVATE libcef_dll_wrapper libcef_lib ${CEF_STANDARD_LIBS})
  SET_EXECUTABLE_TARGET_PROPERTIES(goreecloud_browser)

  if(OS_LINUX OR OS_WINDOWS)
    SET_CEF_TARGET_OUT_DIR()
    COPY_FILES(goreecloud_browser "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "$<TARGET_FILE_DIR:goreecloud_browser>")
    COPY_FILES(goreecloud_browser "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "$<TARGET_FILE_DIR:goreecloud_browser>")
  endif()
endif()

if(GOREECLOUD_ENABLE_LINUX_GTK_HOST)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0 gdk-x11-3.0)
  target_sources(goreecloud_browser PRIVATE
    src/platform/gtk_linux_glaze_host.cpp
    src/platform/gtk_media_hover_popover.cpp
    src/platform/gtk_media_action_host.cpp
  )
  target_link_libraries(goreecloud_browser PRIVATE PkgConfig::GTK3)
  target_compile_definitions(goreecloud_browser PRIVATE GOREECLOUD_ENABLE_LINUX_GTK_HOST=1)
else()
  target_compile_definitions(goreecloud_browser PRIVATE GOREECLOUD_ENABLE_LINUX_GTK_HOST=0)
endif()

goreecloud_apply_warnings(goreecloud_browser)

add_executable(goreecloud_browser_runtime_smoke apps/browser/runtime_smoke.cpp)
target_link_libraries(goreecloud_browser_runtime_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_runtime_smoke)

add_executable(goreecloud_browser_glaze_contract_smoke apps/browser/glaze_contract_smoke.cpp)
target_link_libraries(goreecloud_browser_glaze_contract_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_glaze_contract_smoke)

add_executable(goreecloud_browser_media_hover_smoke apps/browser/media_hover_smoke.cpp)
target_link_libraries(goreecloud_browser_media_hover_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_media_hover_smoke)

add_executable(goreecloud_browser_download_manager_smoke apps/browser/download_manager_smoke.cpp)
target_link_libraries(goreecloud_browser_download_manager_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_download_manager_smoke)

add_executable(goreecloud_browser_download_storage_smoke apps/browser/download_storage_smoke.cpp)
target_link_libraries(goreecloud_browser_download_storage_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_download_storage_smoke)

add_executable(goreecloud_browser_download_runtime_smoke apps/browser/download_runtime_smoke.cpp)
target_link_libraries(goreecloud_browser_download_runtime_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_download_runtime_smoke)

add_executable(goreecloud_browser_wardveil_download_security_smoke
  apps/browser/wardveil_download_security_smoke.cpp)
target_link_libraries(goreecloud_browser_wardveil_download_security_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_wardveil_download_security_smoke)

add_executable(goreecloud_browser_session_recovery_smoke
  apps/browser/session_recovery_smoke.cpp)
target_link_libraries(goreecloud_browser_session_recovery_smoke PRIVATE goreecloud_browser_core)
goreecloud_apply_warnings(goreecloud_browser_session_recovery_smoke)

if(GOREECLOUD_ENABLE_CHROMIUM)
  add_executable(goreecloud_browser_chromium_smoke apps/browser/chromium_smoke.cpp)
  target_link_libraries(goreecloud_browser_chromium_smoke PRIVATE goreecloud_browser_core goreecloud_browser_chromium)
  if(GOREECLOUD_ENABLE_CEF)
    SET_EXECUTABLE_TARGET_PROPERTIES(goreecloud_browser_chromium_smoke)
  endif()
  goreecloud_apply_warnings(goreecloud_browser_chromium_smoke)
endif()

if(BUILD_TESTING)
  add_test(NAME goreecloud_browser_runtime_smoke COMMAND goreecloud_browser_runtime_smoke)
  add_test(NAME goreecloud_browser_glaze_contract_smoke COMMAND goreecloud_browser_glaze_contract_smoke)
  add_test(NAME goreecloud_browser_media_hover_smoke COMMAND goreecloud_browser_media_hover_smoke)
  add_test(NAME goreecloud_browser_download_manager_smoke COMMAND goreecloud_browser_download_manager_smoke)
  add_test(NAME goreecloud_browser_download_storage_smoke COMMAND goreecloud_browser_download_storage_smoke)
  add_test(NAME goreecloud_browser_download_runtime_smoke COMMAND goreecloud_browser_download_runtime_smoke)
  add_test(NAME goreecloud_browser_wardveil_download_security_smoke
           COMMAND goreecloud_browser_wardveil_download_security_smoke)
  add_test(NAME goreecloud_browser_session_recovery_smoke
           COMMAND goreecloud_browser_session_recovery_smoke)
  if(GOREECLOUD_ENABLE_CHROMIUM AND NOT GOREECLOUD_ENABLE_CEF)
    add_test(NAME goreecloud_browser_chromium_smoke COMMAND goreecloud_browser_chromium_smoke)
  endif()
endif()

install(TARGETS goreecloud_browser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/linux/io.goreecloud.Browser.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/linux/io.goreecloud.Browser.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES assets/branding/goreecloud-browser.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME io.goreecloud.Browser.svg)

message(STATUS "GoreeCloud Browser ${PROJECT_VERSION} channel=${GOREECLOUD_RELEASE_CHANNEL}")
message(STATUS "GLAZE UI Stable ${GOREECLOUD_GLAZE_UI_STABLE_VERSION} @ ${GOREECLOUD_GLAZE_UI_STABLE_REVISION}")
message(STATUS "Chromium adapter: ${GOREECLOUD_ENABLE_CHROMIUM}")
message(STATUS "CEF runtime: ${GOREECLOUD_ENABLE_CEF}")
message(STATUS "Linux GTK beta host: ${GOREECLOUD_ENABLE_LINUX_GTK_HOST}")
message(STATUS "libcurl download transport: ${GOREECLOUD_ENABLE_CURL_DOWNLOAD_TRANSPORT}")
