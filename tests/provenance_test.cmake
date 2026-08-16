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
    "51147aaa18a990588f391a491a43048659888631"
    "3f159b13e000ca9f0906599c9c5fd9c13f55233c")
  string(FIND "${third_party}" "${pin}" pin_offset)
  if(pin_offset EQUAL -1)
    message(FATAL_ERROR "missing source pin: ${pin}")
  endif()
endforeach()

set(pinned_files
  "src/portable/fiat/curve25519_64.c|645233c37707ba0580338aa84d8380357078a2c9bb2db80f0c5ff4e979650e3e"
  "src/arm64/x25519_aarch64.S|2d267ce802f44c839adce339d823bbbe491462b312cdc36e3fa1b3ebc00870e6"
  "src/x86_64/s2n/_internal_s2n_bignum.h|af14486ec593c27670bfa7cb74057e2116b1cf8d9945cc2bdc736b61f2accc96"
  "src/x86_64/s2n/_internal_s2n_bignum_x86.h|32e93f25e504f9aef6d16250e4eaf0e6214ca00c6312d44d909d6be13a0c249d"
  "src/x86_64/s2n/s2n-bignum.h|949d80230b3a831187e52925d9f3b0479084072dc9a05657ca340d4daea070c9"
  "src/x86_64/s2n/curve25519_pxscalarmul.S|abe6ceed71920248f01d2d98b9676d2d307cf85f563ca1bf075dafc116c2dd62"
  "src/x86_64/s2n/curve25519_pxscalarmul_alt.S|7ae6abca234cdbf6fd388cd661d64028fbe0b65158be7be2e9d39080542b11eb"
  "src/x86_64/s2n/bignum_inv_p25519.S|62e6957a927857a63bb0e705ad96d7ca9c7693c482a7d83621ac619dd24c2430"
  "src/x86_64/s2n/bignum_mod_p25519_4.S|cca0ae2bb193643b0fc42918d007eeb0d93e4aeaed9a843da35ef77aca3ee818"
  "src/x86_64/s2n/bignum_mul_p25519.S|18e3ee6bd465dc22bf151a20575d7194428e995b556e56552d8ed977e853b672"
  "src/x86_64/s2n/bignum_mul_p25519_alt.S|03c8c03f44cf34b702c4010ebbdde9338926273e311e312e7299e4c173f566ab"
  "src/fixed_base/ref10/base.h|dac873e5965b34e29a14b56bc63f9434fbba16b3513daf53c7b0b2d07b614ac0"
  "src/fixed_base/ref10/crypto_int32.h|238803073441714ec17cd8b5f499f697f3dced53951daec0746eb79be45693d7"
  "src/fixed_base/ref10/crypto_int64.h|84789af2e9d29ee05909fbfd4fca6e53066a99b170494d56bf40d0322f98aa67"
  "src/fixed_base/ref10/crypto_uint32.h|13f943899ccdbaf3b50fdab4658a1ca67d59abdb17c08a6be6ed31a30378556d"
  "src/fixed_base/ref10/fe.h|3e9634fb94383fdfbcb85dd4fd961ce5cc03205c2d373dad6ca9314471c3fbef"
  "src/fixed_base/ref10/fe_0.c|825fc6d85b392d15c9b26c26ad2ccfb664a1c09658d8e2d26c54e37ac6e8b509"
  "src/fixed_base/ref10/fe_1.c|a7ab32148848f7c44bc78b1e6d53c7bd633f629694b3b2dc075cc06d5eb17054"
  "src/fixed_base/ref10/fe_add.c|f838729610a80bb3a988fdfe1a8a9e9ab01648006492adf1ff35b544a38cf398"
  "src/fixed_base/ref10/fe_cmov.c|dc5f5993f3268ee20a333407e2d4b7a26a38ce1f8be7ce20b11c7d8f05efe03d"
  "src/fixed_base/ref10/fe_copy.c|0f91f42eac64053f508537fff726561d2e21cd88f5f7fa304f34f419545a9121"
  "src/fixed_base/ref10/fe_invert.c|504b694c38ab523f9514164a6f19dfa2987e5561525e4ebb1964c33028b0a6fb"
  "src/fixed_base/ref10/fe_mul.c|ca0e7e4dd7e28d9400e4b7f252f2da8f85902a5a07656fee889e18fa28a89fbb"
  "src/fixed_base/ref10/fe_neg.c|3ce85c334bbe9fa2e7e231d2319ec3d29cd03781c7b3108570635348652732c5"
  "src/fixed_base/ref10/fe_sq.c|daf664ebbcdf83aed80889ba11aca06eb1ec3e958ce8c84322a54a49ff9891c3"
  "src/fixed_base/ref10/fe_sq2.c|f3fc9e467f288c0b3094e208904b7ab1f8465d17f81dde240a7e91f865e0e8ba"
  "src/fixed_base/ref10/fe_sub.c|1a2afa4c5fc5fcec3dfd1344dbb41ba3b1c93acea47f74b413216b9c8f374c11"
  "src/fixed_base/ref10/fe_tobytes.c|b2b98c956161eca89c3e8504258d39d77bd09189ff774f0f9300a7948f0832ed"
  "src/fixed_base/ref10/ge.h|b29fe29692432a860689bede278dd2d20bafb52fa6534db30ca781e285e5c495"
  "src/fixed_base/ref10/ge_madd.c|daf288d47d4d7356821912391ad2279a7ae5696a28bde7830b1660672ab5e6b4"
  "src/fixed_base/ref10/ge_madd.h|a39615f652006291a3fb0299728243191937b98e0e91fc8bf8196f96075a8d60"
  "src/fixed_base/ref10/ge_p1p1_to_p2.c|be6e9b143c304c652534eea0fa9c92031005db7c83b824e7cf95581c093f9983"
  "src/fixed_base/ref10/ge_p1p1_to_p3.c|ce67bc193d4d9c81ff18532ab5c1e74ccf519e8349ed19ff112feb22df27a3d2"
  "src/fixed_base/ref10/ge_p2_dbl.c|3d6aba9a349542b8350c5bbc8f69252d463b14605ad65d35241e89812482e780"
  "src/fixed_base/ref10/ge_p2_dbl.h|68aece7c0d6d99f8c6a91b4708d85336f060f9dfe171c1484097c4e380d1f884"
  "src/fixed_base/ref10/ge_p3_0.c|e11b72f763151ef2ce652a0bf7991673b01e6ce74f15e90fd4af28cde505a52b"
  "src/fixed_base/ref10/ge_p3_dbl.c|343ec8eddef9d91d130e3bbf443cdd64d226cd129530fede2126a837e31aab49"
  "src/fixed_base/ref10/ge_p3_to_p2.c|1a29bfa686815c7c694d6cd2c660314f00e2bc53278478b7d8d51ede5133c34d"
  "src/fixed_base/ref10/ge_precomp_0.c|602db7879544d2084b3a6b21b33950b2b6974fabc36a8c867ad7207ac7a7b115"
  "src/fixed_base/ref10/ge_scalarmult_base.c|ff4f6229f538683e3cd1c6767b16b6c6002c2fc1bc1af47dd069fe1aeff40b18"
  "src/fixed_base/ref10/pow225521.h|7ab9170e5237fa676da2acd90cfe72a0357852114aeb937e210dc7dd6b9ff3df"
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
