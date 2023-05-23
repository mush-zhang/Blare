# find_package(pcre2-8 pcre2-16 pcre2-32 REQUIRED)
# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/pcre2
find_library(NAMES pcre2-8 pcre2-16 pcre2-32)

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_pcre2/*.hpp")

add_library(blare_pcre2_lib 
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST}
    # "/usr/local/include/jpcre2.hpp")
    ${CMAKE_BINARY_DIR}/_deps/jpcre2.hpp)

target_include_directories(blare_pcre2_lib PUBLIC "/usr/local/include" "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(blare_pcre2_lib PRIVATE misc_lib Boost::boost Threads::Threads -lpcre2-8 -lpcre2-16 -lpcre2-32)

target_compile_features(blare_pcre2_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_pcre2"
  PREFIX "BLARE PCRE2 Header Files"
  FILES ${HEADER_LIST})
