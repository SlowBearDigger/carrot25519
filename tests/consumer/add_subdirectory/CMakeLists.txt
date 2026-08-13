cmake_minimum_required(VERSION 3.20)

project(carrot25519_add_subdirectory_consumer LANGUAGES C CXX)

if(NOT DEFINED CARROT25519_SOURCE_DIR)
  message(FATAL_ERROR "CARROT25519_SOURCE_DIR is required")
endif()

add_subdirectory("${CARROT25519_SOURCE_DIR}" carrot25519)
add_executable(consumer main.cpp)
target_compile_features(consumer PRIVATE cxx_std_17)
target_link_libraries(consumer PRIVATE carrot25519::carrot25519)

enable_testing()
add_test(NAME consumer COMMAND consumer)
