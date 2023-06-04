find_package(re2 REQUIRED)

# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/re2
find_library(NAMES re2)

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_re2/*.hpp")

add_library(blare_re2_lib 
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST})

target_include_directories(blare_re2_lib PUBLIC "/usr/local/include" "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(blare_re2_lib PUBLIC misc_lib Boost::boost re2::re2 Threads::Threads)

target_compile_features(blare_re2_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_re2"
  PREFIX "BLARE RE2 Header Files"
  FILES ${HEADER_LIST})

