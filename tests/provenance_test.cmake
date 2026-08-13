if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT is required")
endif()

set(required_files
  LICENSE
  SECURITY.md
  THIRD_PARTY.md
  licenses/CC0-1.0.txt
  licenses/MIT-FIAT.txt
  licenses/MIT-0-S2N.txt
  licenses/NOTICE-S2N.txt)

foreach(path IN LISTS required_files)
  if(NOT EXISTS "${PROJECT_ROOT}/${path}")
    message(FATAL_ERROR "missing required file: ${path}")
  endif()
endforeach()

execute_process(
  COMMAND git -C "${PROJECT_ROOT}" ls-files --cached --others --exclude-standard
  RESULT_VARIABLE inventory_status
  OUTPUT_VARIABLE inventory_text
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT inventory_status EQUAL 0)
  message(FATAL_ERROR "unable to read repository inventory")
endif()

string(REPLACE "\n" ";" inventory "${inventory_text}")
foreach(path IN LISTS inventory)
  if(NOT path STREQUAL "tests/provenance_test.cmake" AND
     NOT IS_DIRECTORY "${PROJECT_ROOT}/${path}")
    file(READ "${PROJECT_ROOT}/${path}" contents LIMIT 1048576)
    if(contents MATCHES "SPDX-License-Identifier:[^\n]*(LGPL|GPL|AGPL)")
      message(FATAL_ERROR "copyleft source identifier: ${path}")
    endif()
  endif()
endforeach()

file(READ "${PROJECT_ROOT}/THIRD_PARTY.md" third_party)
foreach(pin IN ITEMS
    "046758072159ee093e837ab6840cde89b9795997"
    "59b13b2f7dc615eccc1f14d14ef05662d9ab0fdd"
    "51147aaa18a990588f391a491a43048659888631")
  string(FIND "${third_party}" "${pin}" pin_offset)
  if(pin_offset EQUAL -1)
    message(FATAL_ERROR "missing source pin: ${pin}")
  endif()
endforeach()

set(pinned_files
  "src/portable/fiat/curve25519_64.c|645233c37707ba0580338aa84d8380357078a2c9bb2db80f0c5ff4e979650e3e"
  "src/arm64/x25519_aarch64.S|9c4814e489a2b1961867778beb139373d57c6d7a7102916f671d0b2cb668d04d"
  "tests/vectors.tsv|2ca612e3879cd8bd2f85e7baae54bb29be4aef5b0f7eb2c4ab94523b3f498ac1")
foreach(binding IN LISTS pinned_files)
  string(REPLACE "|" ";" fields "${binding}")
  list(GET fields 0 path)
  list(GET fields 1 expected_sha256)
  if(NOT EXISTS "${PROJECT_ROOT}/${path}")
    message(FATAL_ERROR "missing pinned file: ${path}")
  endif()
  file(SHA256 "${PROJECT_ROOT}/${path}" actual_sha256)
  if(NOT actual_sha256 STREQUAL expected_sha256)
    message(FATAL_ERROR "pinned file mismatch: ${path}")
  endif()
endforeach()

message(STATUS "provenance check passed")
