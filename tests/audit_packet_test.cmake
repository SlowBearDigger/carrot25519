if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT is required")
endif()

foreach(path IN ITEMS AUDIT.md tools/review.sh)
  if(NOT EXISTS "${PROJECT_ROOT}/${path}")
    message(FATAL_ERROR "missing audit packet file: ${path}")
  endif()
endforeach()

file(READ "${PROJECT_ROOT}/AUDIT.md" audit)
file(READ "${PROJECT_ROOT}/tools/review.sh" runner)
string(REGEX REPLACE "[\r\n\t ]+" " " audit_normalized "${audit}")

foreach(required IN ITEMS
    "carrot25519_select_impl"
    "carrot25519_impl_id_of"
    "carrot25519_impl_name"
    "carrot25519_mul_base"
    "carrot25519_mul"
    "does not validate points"
    "does not reject an all-zero result"
    "Native Linux x86_64"
    "Native Linux AArch64"
    "not independently audited"
    "not a proof of constant-time behavior"
    "src/portable/portable.c"
    "src/arm64/x25519_aarch64.S"
    "src/x86_64/x86_64.c")
  string(FIND "${audit_normalized}" "${required}" offset)
  if(offset EQUAL -1)
    message(FATAL_ERROR "audit packet is missing: ${required}")
  endif()
endforeach()

foreach(required IN ITEMS
    "CMAKE_BUILD_TYPE=Release"
    "fsanitize=address"
    "fsanitize=undefined"
    "ctest"
    "dirty_state="
    "workflow_policy_test.py")
  string(FIND "${runner}" "${required}" offset)
  if(offset EQUAL -1)
    message(FATAL_ERROR "review runner is missing: ${required}")
  endif()
endforeach()

foreach(forbidden IN ITEMS "curl " "wget " "git clone")
  string(FIND "${runner}" "${forbidden}" offset)
  if(NOT offset EQUAL -1)
    message(FATAL_ERROR "review runner contains network operation: ${forbidden}")
  endif()
endforeach()

message(STATUS "audit packet contract passed")
