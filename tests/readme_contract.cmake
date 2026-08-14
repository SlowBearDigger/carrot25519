if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT is required")
endif()

file(READ "${PROJECT_ROOT}/README.md" readme)
string(TOLOWER "${readme}" normalized_readme)

foreach(required IN ITEMS
    "experimental"
    "not audited"
    "not production-ready"
    "scalar bits 0 through 254"
    "bit 255 is ignored"
    "portable"
    "ARM64"
    "x86_64"
    "Local Linux AArch64 VM"
    "Bare-metal Linux AArch64"
    "Windows is unsupported"
    "CARROT"
    "MIT"
    "CC0-1.0"
    "MIT-0"
    "85qhvVeJwqd7LUhivp4YchfTQRCqs51GHaF13kkSgNLnBNtrnNVvADGVTvSUYKMDbfSitivYkZC39DwLByKBAWq9Gb38ggo")
  string(TOLOWER "${required}" normalized_required)
  string(FIND "${normalized_readme}" "${normalized_required}" offset)
  if(offset EQUAL -1)
    message(FATAL_ERROR "README is missing required text: ${required}")
  endif()
endforeach()

foreach(unsupported IN ITEMS
    "formally verified"
    "production ready"
    "universally constant-time")
  string(FIND "${normalized_readme}" "${unsupported}" offset)
  if(NOT offset EQUAL -1)
    message(FATAL_ERROR "README contains unsupported claim: ${unsupported}")
  endif()
endforeach()

message(STATUS "README contract passed")
