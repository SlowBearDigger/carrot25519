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
    "GitHub-hosted Linux"
    "31805437243"
    "Windows is unsupported"
    "CARROT"
    "MIT licensed"
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
    "universally constant-time"
    "Native Linux qualification remains pending"
    "native x86_64 benchmark remains pending"
    "permissively licensed"
    "No LGPL code"
    "MIT-0 terms")
  string(FIND "${normalized_readme}" "${unsupported}" offset)
  if(NOT offset EQUAL -1)
    message(FATAL_ERROR "README contains unsupported claim: ${unsupported}")
  endif()
endforeach()

message(STATUS "README contract passed")
