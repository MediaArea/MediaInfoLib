# Try to find the Brotli library
# Adapted from https://github.com/curl/curl/blob/master/CMake/FindBrotli.cmake

find_path(BROTLI_INCLUDE_DIR "brotli/decode.h")
find_library(BROTLICOMMON_LIBRARY NAMES "brotlicommon")
find_library(BROTLIDEC_LIBRARY NAMES "brotlidec")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brotli
  REQUIRED_VARS
    BROTLI_INCLUDE_DIR
    BROTLIDEC_LIBRARY
    BROTLICOMMON_LIBRARY
)

if(BROTLI_FOUND)
  set(BROTLI_INCLUDE_DIRS ${BROTLI_INCLUDE_DIR})
  set(BROTLI_LIBRARIES ${BROTLIDEC_LIBRARY} ${BROTLICOMMON_LIBRARY})
  include_directories(${BROTLI_INCLUDE_DIRS})
endif()

mark_as_advanced(BROTLI_INCLUDE_DIR BROTLIDEC_LIBRARY BROTLICOMMON_LIBRARY)
