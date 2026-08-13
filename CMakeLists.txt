cmake_minimum_required(VERSION 3.20)

project(carrot25519 VERSION 0.1.0 LANGUAGES C ASM)

option(CARROT25519_BUILD_TESTS "Build carrot25519 tests" OFF)
option(CARROT25519_BUILD_BENCHMARKS "Build carrot25519 benchmarks" OFF)
option(CARROT25519_PORTABLE_ONLY "Disable optimized backends" OFF)

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

if(CARROT25519_BUILD_TESTS)
  enable_testing()

  add_executable(carrot25519_api_contract_test tests/api_contract_test.c)
  target_link_libraries(carrot25519_api_contract_test PRIVATE carrot25519)
  add_test(NAME api_contract COMMAND carrot25519_api_contract_test)

  add_executable(carrot25519_corpus_test tests/corpus_test.c)
  target_link_libraries(carrot25519_corpus_test PRIVATE carrot25519)
  add_test(
    NAME corpus
    COMMAND carrot25519_corpus_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/vectors.tsv)

  add_test(
    NAME provenance
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/provenance_test.cmake)
endif()
