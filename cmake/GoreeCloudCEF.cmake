# GoreeCloud Browser — pinned CEF acquisition
#
# Network acquisition is deliberately opt-in. The authoritative source version is
# reviewed in this file; callers cannot silently select a different CEF build from
# a command-line cache variable.
#
# CEF's official sample project downloads binary distributions from
# https://cef-builds.spotifycdn.com and verifies the upstream-published SHA-1
# sidecar before extraction. GoreeCloud follows that upstream acquisition
# contract for Development builds while treating it as transport/integrity
# evidence only, not as production supply-chain acceptance.

set(GOREECLOUD_CEF_STABLE_VERSION
    "152.0.6+g708dc14+chromium-152.0.7977.83")
set(GOREECLOUD_CEF_STABLE_CHROMIUM_VERSION
    "152.0.7977.83")
set(GOREECLOUD_CEF_OFFICIAL_BASE_URL
    "https://cef-builds.spotifycdn.com")

function(goreecloud_prepare_pinned_cef download_dir out_root)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
      "GoreeCloud automatic CEF acquisition is currently reviewed only for Linux x86_64.")
  endif()

  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" goreecloud_cef_processor)
  if(NOT goreecloud_cef_processor MATCHES "^(x86_64|amd64)$")
    message(FATAL_ERROR
      "GoreeCloud automatic CEF acquisition currently supports Linux x86_64 only; "
      "got CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}.")
  endif()

  set(platform "linux64")
  set(distribution
      "cef_binary_${GOREECLOUD_CEF_STABLE_VERSION}_${platform}")
  set(root "${download_dir}/${distribution}")
  set(archive "${download_dir}/${distribution}.tar.bz2")
  set(sidecar "${archive}.sha1")

  if(EXISTS "${root}/include/cef_app.h" AND
     EXISTS "${root}/cmake/FindCEF.cmake")
    message(STATUS
      "Using cached GoreeCloud-pinned CEF Stable ${GOREECLOUD_CEF_STABLE_VERSION} at ${root}")
    set(${out_root} "${root}" PARENT_SCOPE)
    return()
  endif()

  file(MAKE_DIRECTORY "${download_dir}")

  set(download_url
      "${GOREECLOUD_CEF_OFFICIAL_BASE_URL}/${distribution}.tar.bz2")
  string(REPLACE "+" "%2B" escaped_download_url "${download_url}")

  message(STATUS
    "Fetching official CEF integrity sidecar for pinned Stable ${GOREECLOUD_CEF_STABLE_VERSION}")
  file(
    DOWNLOAD
      "${escaped_download_url}.sha1"
      "${sidecar}"
    TLS_VERIFY ON
    STATUS sidecar_status
    LOG sidecar_log
  )
  list(GET sidecar_status 0 sidecar_code)
  list(GET sidecar_status 1 sidecar_message)
  if(NOT sidecar_code EQUAL 0)
    file(REMOVE "${sidecar}")
    message(FATAL_ERROR
      "Failed to obtain the official CEF SHA-1 sidecar: ${sidecar_message}\n${sidecar_log}")
  endif()

  file(READ "${sidecar}" expected_sha1)
  string(STRIP "${expected_sha1}" expected_sha1)
  string(LENGTH "${expected_sha1}" expected_sha1_length)
  if(NOT expected_sha1_length EQUAL 40 OR
     NOT expected_sha1 MATCHES "^[0-9A-Fa-f]+$")
    file(REMOVE "${sidecar}")
    message(FATAL_ERROR
      "Official CEF integrity sidecar was malformed for ${distribution}.")
  endif()

  set(download_required TRUE)
  if(EXISTS "${archive}")
    file(SHA1 "${archive}" cached_sha1)
    string(TOLOWER "${cached_sha1}" cached_sha1)
    string(TOLOWER "${expected_sha1}" expected_sha1_lower)
    if(cached_sha1 STREQUAL expected_sha1_lower)
      set(download_required FALSE)
      message(STATUS "Verified cached CEF archive SHA-1 ${cached_sha1}")
    else()
      message(WARNING "Discarding cached CEF archive with mismatched integrity hash.")
      file(REMOVE "${archive}")
    endif()
  endif()

  if(download_required)
    set(partial "${archive}.part")
    file(REMOVE "${partial}")
    message(STATUS
      "Downloading pinned CEF Stable ${GOREECLOUD_CEF_STABLE_VERSION} from the official CEF CDN")
    file(
      DOWNLOAD
        "${escaped_download_url}"
        "${partial}"
      EXPECTED_HASH "SHA1=${expected_sha1}"
      TLS_VERIFY ON
      STATUS archive_status
      LOG archive_log
      SHOW_PROGRESS
    )
    list(GET archive_status 0 archive_code)
    list(GET archive_status 1 archive_message)
    if(NOT archive_code EQUAL 0)
      file(REMOVE "${partial}")
      message(FATAL_ERROR
        "Failed to download/verify pinned CEF Stable: ${archive_message}\n${archive_log}")
    endif()
    file(RENAME "${partial}" "${archive}")
  endif()

  file(SHA256 "${archive}" archive_sha256)
  message(STATUS "Verified pinned CEF archive SHA-1: ${expected_sha1}")
  message(STATUS "Observed pinned CEF archive SHA-256: ${archive_sha256}")

  file(REMOVE_RECURSE "${root}")
  message(STATUS "Extracting pinned CEF Stable into ${download_dir}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xzf "${archive}"
    WORKING_DIRECTORY "${download_dir}"
    RESULT_VARIABLE extract_result
    ERROR_VARIABLE extract_error
  )
  if(NOT extract_result EQUAL 0)
    file(REMOVE_RECURSE "${root}")
    message(FATAL_ERROR
      "Failed to extract pinned CEF Stable: ${extract_error}")
  endif()

  if(NOT EXISTS "${root}/include/cef_app.h" OR
     NOT EXISTS "${root}/cmake/FindCEF.cmake")
    file(REMOVE_RECURSE "${root}")
    message(FATAL_ERROR
      "Pinned CEF extraction is incomplete or has an unexpected layout: ${root}")
  endif()

  set(${out_root} "${root}" PARENT_SCOPE)
endfunction()
