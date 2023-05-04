# Find the installed pcre2 library
# find_package(pcre2-8 pcre2-16 pcre2-32 REQUIRED)
# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:
# # inside myproject/build
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/pcre2
find_library(NAMES pcre2-8 pcre2-16 pcre2-32)

# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_pcre2/*.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(blare_pcre2_lib 
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST}
    "/usr/local/include/jpcre2.hpp")

# We need this directory, and users of our library will need it too
target_include_directories(blare_pcre2_lib PUBLIC "/usr/local/include" "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(blare_pcre2_lib PRIVATE misc_lib Boost::boost)

# All users of this library will need at least C++17
target_compile_features(blare_pcre2_lib PUBLIC cxx_std_17)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_pcre2"
  PREFIX "BLARE PCRE2 Header Files"
  FILES ${HEADER_LIST})
