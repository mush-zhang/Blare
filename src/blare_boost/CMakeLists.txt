find_package(Boost REQUIRED COMPONENTS regex) 
# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:
# # inside myproject/build
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/boost
find_library(NAMES boost_regex)

# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_boost/*.hpp")
# set(HEADER_LIST "${BLARE_SOURCE_DIR}/include/blare_boost/lib.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(blare_boost_lib 
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(blare_boost_lib PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

# This depends on (header only) boost
target_link_libraries(blare_boost_lib PRIVATE misc_lib Boost::boost Boost::regex) 


# All users of this library will need at least C++17
target_compile_features(blare_boost_lib PUBLIC cxx_std_17)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_boost"
  PREFIX "BLARE Boost Header Files"
  FILES ${HEADER_LIST})
