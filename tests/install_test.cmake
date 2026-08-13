if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT DEFINED PROJECT_BINARY_DIR OR
   NOT DEFINED CMAKE_EXECUTABLE OR NOT DEFINED CTEST_EXECUTABLE)
  message(FATAL_ERROR "install test inputs are required")
endif()

set(test_root "${PROJECT_BINARY_DIR}/install-consumer")
set(prefix "${test_root}/prefix")
set(consumer_build "${test_root}/build")
file(REMOVE_RECURSE "${test_root}")
set(build_config_args)
set(test_config_args)
if(DEFINED PROJECT_CONFIG AND NOT PROJECT_CONFIG STREQUAL "")
  list(APPEND build_config_args --config "${PROJECT_CONFIG}")
  list(APPEND test_config_args -C "${PROJECT_CONFIG}")
endif()

execute_process(
  COMMAND "${CMAKE_EXECUTABLE}" --install "${PROJECT_BINARY_DIR}"
          --prefix "${prefix}" ${build_config_args}
  RESULT_VARIABLE install_status)
if(NOT install_status EQUAL 0)
  message(FATAL_ERROR "project installation failed")
endif()

execute_process(
  COMMAND "${CMAKE_EXECUTABLE}"
          -S "${PROJECT_SOURCE_DIR}/tests/consumer/install"
          -B "${consumer_build}"
          "-DCMAKE_PREFIX_PATH=${prefix}"
  RESULT_VARIABLE configure_status)
if(NOT configure_status EQUAL 0)
  message(FATAL_ERROR "consumer configuration failed")
endif()

execute_process(
  COMMAND "${CMAKE_EXECUTABLE}" --build "${consumer_build}" --parallel
          ${build_config_args}
  RESULT_VARIABLE build_status)
if(NOT build_status EQUAL 0)
  message(FATAL_ERROR "consumer build failed")
endif()

execute_process(
  COMMAND "${CTEST_EXECUTABLE}" --test-dir "${consumer_build}"
          --output-on-failure ${test_config_args}
  RESULT_VARIABLE test_status)
if(NOT test_status EQUAL 0)
  message(FATAL_ERROR "installed consumer test failed")
endif()
