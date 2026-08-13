cmake_minimum_required(VERSION 3.20)

project(carrot25519 VERSION 0.1.0 LANGUAGES C ASM)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(CARROT25519_BUILD_TESTS "Build carrot25519 tests" OFF)
option(CARROT25519_BUILD_BENCHMARKS "Build carrot25519 benchmarks" OFF)
option(CARROT25519_PORTABLE_ONLY "Disable optimized backends" OFF)

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" carrot25519_processor)
set(CARROT25519_TARGET_ARM64 OFF)
if(NOT CARROT25519_PORTABLE_ONLY AND
   carrot25519_processor MATCHES "^(arm64|aarch64)$")
  set(CARROT25519_TARGET_ARM64 ON)
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

  add_executable(carrot25519_backend_matrix_test tests/backend_matrix_test.c)
  target_link_libraries(carrot25519_backend_matrix_test PRIVATE carrot25519)
  carrot25519_strict_warnings(carrot25519_backend_matrix_test)
  if(CARROT25519_TARGET_ARM64)
    target_compile_definitions(carrot25519_backend_matrix_test
      PRIVATE CARROT25519_TEST_EXPECT_ARM64=1)
  endif()
  add_test(NAME backend_matrix COMMAND carrot25519_backend_matrix_test)

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
endif()
