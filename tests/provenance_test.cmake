if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT is required")
endif()

set(required_files
  AUDIT.md
  LICENSE
  README.md
  SECURITY.md
  THIRD_PARTY.md
  licenses/CC0-1.0.txt
  licenses/MIT-FIAT.txt
  licenses/MIT-0-S2N.txt
  licenses/NOTICE-S2N.txt
  tools/review.sh)

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
     EXISTS "${PROJECT_ROOT}/${path}" AND
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
  "src/x86_64/s2n/_internal_s2n_bignum.h|af14486ec593c27670bfa7cb74057e2116b1cf8d9945cc2bdc736b61f2accc96"
  "src/x86_64/s2n/_internal_s2n_bignum_x86.h|32e93f25e504f9aef6d16250e4eaf0e6214ca00c6312d44d909d6be13a0c249d"
  "src/x86_64/s2n/s2n-bignum.h|949d80230b3a831187e52925d9f3b0479084072dc9a05657ca340d4daea070c9"
  "src/x86_64/s2n/curve25519_pxscalarmul.S|abe6ceed71920248f01d2d98b9676d2d307cf85f563ca1bf075dafc116c2dd62"
  "src/x86_64/s2n/curve25519_pxscalarmul_alt.S|7ae6abca234cdbf6fd388cd661d64028fbe0b65158be7be2e9d39080542b11eb"
  "src/x86_64/s2n/bignum_inv_p25519.S|62e6957a927857a63bb0e705ad96d7ca9c7693c482a7d83621ac619dd24c2430"
  "src/x86_64/s2n/bignum_mod_p25519_4.S|cca0ae2bb193643b0fc42918d007eeb0d93e4aeaed9a843da35ef77aca3ee818"
  "src/x86_64/s2n/bignum_mul_p25519.S|18e3ee6bd465dc22bf151a20575d7194428e995b556e56552d8ed977e853b672"
  "src/x86_64/s2n/bignum_mul_p25519_alt.S|03c8c03f44cf34b702c4010ebbdde9338926273e311e312e7299e4c173f566ab"
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
