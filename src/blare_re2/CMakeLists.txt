# Find the installed re2 library
find_package(re2 REQUIRED)
# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:

# # inside myproject/build
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/re2

# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_re2/*.hpp")
# set(HEADER_LIST "${BLARE_SOURCE_DIR}/include/blare_re2/lib.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(blare_re2_lib 
    MODULE
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(blare_re2_lib PUBLIC "/usr/local/include" "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

# This depends on (header only) boost
target_link_libraries(blare_re2_lib PRIVATE misc_lib Boost::boost re2::re2)

# All users of this library will need at least C++17
target_compile_features(blare_re2_lib PUBLIC cxx_std_17)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_re2"
  PREFIX "BLARE RE2 Header Files"
  FILES ${HEADER_LIST})

