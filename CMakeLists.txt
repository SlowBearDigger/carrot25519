cmake_minimum_required(VERSION 3.20)

project(carrot25519 VERSION 0.1.0 LANGUAGES C ASM)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(CARROT25519_BUILD_TESTS "Build carrot25519 tests" OFF)
option(CARROT25519_BUILD_BENCHMARKS "Build carrot25519 benchmarks" OFF)
option(CARROT25519_BUILD_MX25519_BENCHMARK
  "Include the external mx25519 reference in benchmarks" OFF)
option(CARROT25519_PORTABLE_ONLY "Disable optimized backends" OFF)
set(CARROT25519_MX25519_SOURCE "" CACHE PATH
  "Path to the exact external mx25519 checkout")
set(CARROT25519_MX25519_COMMIT "" CACHE STRING
  "Expected external mx25519 commit")
set(CARROT25519_MX25519_TREE "" CACHE STRING
  "Expected external mx25519 tree")

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" carrot25519_processor)
set(CARROT25519_TARGET_ARM64 OFF)
set(CARROT25519_TARGET_X86_64 OFF)
if(NOT CARROT25519_PORTABLE_ONLY)
  if(APPLE AND CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
    set(CARROT25519_TARGET_X86_64 ON)
  elseif(carrot25519_processor MATCHES "^(arm64|aarch64)$")
    set(CARROT25519_TARGET_ARM64 ON)
  elseif(UNIX AND carrot25519_processor MATCHES "^(x86_64|amd64)$")
    set(CARROT25519_TARGET_X86_64 ON)
  endif()
endif()

function(carrot25519_strict_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -Wpedantic)
  endif()
endfunction()

add_library(carrot25519 STATIC
  src/carrot25519.c
  src/secure_zero.c
  src/portable/portable.c)
add_library(carrot25519::carrot25519 ALIAS carrot25519)

target_compile_features(carrot25519 PUBLIC c_std_11)
target_include_directories(carrot25519
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE src)
set_target_properties(carrot25519 PROPERTIES
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON)

carrot25519_strict_warnings(carrot25519)

if(CARROT25519_TARGET_ARM64)
  target_sources(carrot25519 PRIVATE
    src/arm64/arm64.c
    src/arm64/x25519_aarch64.S)
  target_compile_definitions(carrot25519 PRIVATE CARROT25519_HAVE_ARM64=1)
endif()

if(CARROT25519_TARGET_X86_64)
  target_sources(carrot25519 PRIVATE
    src/x86_64/x86_64.c
    src/x86_64/s2n/curve25519_pxscalarmul.S
    src/x86_64/s2n/curve25519_pxscalarmul_alt.S
    src/x86_64/s2n/bignum_inv_p25519.S
    src/x86_64/s2n/bignum_mod_p25519_4.S
    src/x86_64/s2n/bignum_mul_p25519.S
    src/x86_64/s2n/bignum_mul_p25519_alt.S)
  target_include_directories(carrot25519 PRIVATE src/x86_64/s2n)
  target_compile_definitions(carrot25519 PRIVATE
    CARROT25519_HAVE_X86_64=1
    S2N_BN_HIDE_SYMBOLS=1)
endif()

if(CARROT25519_BUILD_BENCHMARKS)
  if(NOT UNIX)
    message(FATAL_ERROR "benchmarks require a Unix-like host")
  endif()
  add_executable(carrot25519_benchmark bench/benchmark.c)
  target_link_libraries(carrot25519_benchmark PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_benchmark)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" carrot25519_build_type)
  string(CONCAT carrot25519_benchmark_flags
    "${CMAKE_C_FLAGS} " "${CMAKE_C_FLAGS_${carrot25519_build_type}}")
  if(carrot25519_benchmark_flags MATCHES "sanitize")
    target_compile_definitions(carrot25519_benchmark
      PRIVATE CARROT25519_BENCHMARK_SANITIZED=1)
  endif()
  target_compile_definitions(carrot25519_benchmark PRIVATE
    "CARROT25519_BENCHMARK_COMPILER=\"${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}\"")

  if(CARROT25519_BUILD_MX25519_BENCHMARK)
    if(NOT IS_DIRECTORY "${CARROT25519_MX25519_SOURCE}")
      message(FATAL_ERROR "CARROT25519_MX25519_SOURCE must be an exact checkout")
    endif()
    foreach(identity IN ITEMS COMMIT TREE)
      string(LENGTH "${CARROT25519_MX25519_${identity}}" identity_length)
      if(NOT identity_length EQUAL 40 OR
         NOT CARROT25519_MX25519_${identity} MATCHES "^[0-9a-f]+$")
        message(FATAL_ERROR "CARROT25519_MX25519_${identity} must be a SHA-1")
      endif()
    endforeach()
    find_package(Git REQUIRED)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} -C "${CARROT25519_MX25519_SOURCE}"
              rev-parse HEAD^{commit} HEAD^{tree}
      RESULT_VARIABLE mx25519_identity_status
      OUTPUT_VARIABLE mx25519_identity
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE "\n" ";" mx25519_identity "${mx25519_identity}")
    list(LENGTH mx25519_identity mx25519_identity_count)
    if(NOT mx25519_identity_status EQUAL 0 OR
       NOT mx25519_identity_count EQUAL 2)
      message(FATAL_ERROR "unable to verify mx25519 source identity")
    endif()
    list(GET mx25519_identity 0 mx25519_actual_commit)
    list(GET mx25519_identity 1 mx25519_actual_tree)
    if(NOT mx25519_actual_commit STREQUAL CARROT25519_MX25519_COMMIT OR
       NOT mx25519_actual_tree STREQUAL CARROT25519_MX25519_TREE)
      message(FATAL_ERROR "mx25519 source identity mismatch")
    endif()
    execute_process(
      COMMAND ${GIT_EXECUTABLE} -C "${CARROT25519_MX25519_SOURCE}"
              status --porcelain=v1
      RESULT_VARIABLE mx25519_status_result
      OUTPUT_VARIABLE mx25519_status
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT mx25519_status_result EQUAL 0 OR NOT mx25519_status STREQUAL "")
      message(FATAL_ERROR "mx25519 source checkout must be clean")
    endif()

    add_subdirectory(
      "${CARROT25519_MX25519_SOURCE}"
      "${CMAKE_CURRENT_BINARY_DIR}/mx25519"
      EXCLUDE_FROM_ALL)
    target_link_libraries(carrot25519_benchmark PRIVATE mx25519_static)
    target_include_directories(carrot25519_benchmark PRIVATE
      "${CARROT25519_MX25519_SOURCE}/include")
    target_compile_definitions(carrot25519_benchmark PRIVATE
      CARROT25519_HAVE_MX25519_BENCHMARK=1
      "CARROT25519_MX25519_COMMIT=\"${CARROT25519_MX25519_COMMIT}\""
      "CARROT25519_MX25519_TREE=\"${CARROT25519_MX25519_TREE}\"")
  endif()
endif()

install(TARGETS carrot25519
  EXPORT carrot25519Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/carrot25519.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT carrot25519Targets
  FILE carrot25519Targets.cmake
  NAMESPACE carrot25519::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/carrot25519)

configure_package_config_file(
  cmake/carrot25519Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/carrot25519Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/carrot25519)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/carrot25519ConfigVersion.cmake
  COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/carrot25519Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/carrot25519ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/carrot25519)

if(CARROT25519_BUILD_TESTS)
  enable_testing()
  enable_language(CXX)

  add_executable(carrot25519_api_contract_test tests/api_contract_test.c)
  target_link_libraries(carrot25519_api_contract_test PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_api_contract_test)
  add_test(NAME api_contract COMMAND carrot25519_api_contract_test)

  add_executable(carrot25519_corpus_test tests/corpus_test.c)
  target_link_libraries(carrot25519_corpus_test PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_corpus_test)
  add_test(
    NAME corpus
    COMMAND carrot25519_corpus_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/vectors.tsv)

  add_test(
    NAME provenance
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/provenance_test.cmake)
  add_test(
    NAME readme_contract
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/readme_contract.cmake)
  add_test(
    NAME audit_packet
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/audit_packet_test.cmake)
  add_test(
    NAME reference_benchmark_policy
    COMMAND ${CMAKE_COMMAND} -E env
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/reference_benchmark_policy_test.py)

  add_executable(carrot25519_backend_matrix_test tests/backend_matrix_test.c)
  target_link_libraries(carrot25519_backend_matrix_test PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_backend_matrix_test)
  if(CARROT25519_TARGET_ARM64)
    target_compile_definitions(carrot25519_backend_matrix_test
      PRIVATE CARROT25519_TEST_EXPECT_ARM64=1)
  endif()
  if(CARROT25519_TARGET_X86_64)
    target_compile_definitions(carrot25519_backend_matrix_test
      PRIVATE CARROT25519_TEST_EXPECT_X86_64=1)
  endif()
  add_test(NAME backend_matrix COMMAND carrot25519_backend_matrix_test)

  add_executable(
    carrot25519_carrot_convergence_test tests/carrot_convergence_test.c)
  target_link_libraries(carrot25519_carrot_convergence_test PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_carrot_convergence_test)
  add_test(NAME carrot_convergence COMMAND carrot25519_carrot_convergence_test)

  add_executable(
    carrot25519_monero_wrapper_compat_test
    tests/monero_wrapper_compat_test.cpp)
  target_compile_features(carrot25519_monero_wrapper_compat_test
    PRIVATE cxx_std_17)
  target_link_libraries(carrot25519_monero_wrapper_compat_test
    PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_monero_wrapper_compat_test)
  add_test(NAME monero_wrapper COMMAND carrot25519_monero_wrapper_compat_test)

  add_test(
    NAME install_consumer
    COMMAND ${CMAKE_COMMAND}
      -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
      -DPROJECT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
      -DCMAKE_EXECUTABLE=${CMAKE_COMMAND}
      -DCTEST_EXECUTABLE=${CMAKE_CTEST_COMMAND}
      -DPROJECT_CONFIG=$<CONFIG>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/install_test.cmake)

  if(CARROT25519_TARGET_ARM64 AND UNIX)
    add_executable(carrot25519_arm64_guard_page_test tests/guard_page_test.c)
    target_compile_definitions(carrot25519_arm64_guard_page_test
      PRIVATE CARROT25519_TEST_IMPL_ID=CARROT25519_IMPL_ARM64)
    target_link_libraries(carrot25519_arm64_guard_page_test PRIVATE carrot25519)
    carrot25519_strict_warnings(carrot25519_arm64_guard_page_test)
    add_test(NAME arm64_guard_pages COMMAND carrot25519_arm64_guard_page_test)
    add_test(
      NAME arm64_disassembly
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_arm64_disassembly.sh
              $<TARGET_FILE:carrot25519>)
  endif()

  if(CARROT25519_TARGET_X86_64)
    add_executable(carrot25519_x86_64_dispatch_test tests/dispatch_test.c)
    target_include_directories(carrot25519_x86_64_dispatch_test PRIVATE src)
    target_link_libraries(carrot25519_x86_64_dispatch_test PRIVATE carrot25519)
    carrot25519_strict_warnings(carrot25519_x86_64_dispatch_test)
    add_test(NAME x86_64_dispatch COMMAND carrot25519_x86_64_dispatch_test)
    if(UNIX)
      add_executable(carrot25519_x86_64_guard_page_test tests/guard_page_test.c)
      target_compile_definitions(carrot25519_x86_64_guard_page_test
        PRIVATE CARROT25519_TEST_IMPL_ID=CARROT25519_IMPL_X86_64_BASELINE)
      target_link_libraries(carrot25519_x86_64_guard_page_test
        PRIVATE carrot25519)
      carrot25519_strict_warnings(carrot25519_x86_64_guard_page_test)
      add_test(NAME x86_64_guard_pages COMMAND carrot25519_x86_64_guard_page_test)
    endif()
    add_test(
      NAME x86_64_disassembly
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_x86_64_disassembly.sh
              $<TARGET_FILE:carrot25519>)
  endif()
endif()
