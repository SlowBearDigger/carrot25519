cmake_minimum_required(VERSION 3.20)

project(carrot25519_install_consumer LANGUAGES C)

find_package(carrot25519 CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE carrot25519::carrot25519)

enable_testing()
add_test(NAME consumer COMMAND consumer)
