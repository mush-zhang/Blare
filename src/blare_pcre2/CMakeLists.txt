# find_library(NAMES pcre2-8 pcre2-16 pcre2-32)

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_pcre2/*.hpp")

add_library(blare_pcre2_lib 
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST}
    ${CMAKE_BINARY_DIR}/_deps/include/jpcre2.hpp)

target_include_directories(blare_pcre2_lib 
    PUBLIC 
    "/usr/local/include" 
    "${PROJECT_BINARY_DIR}" 
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/_deps/include"
    "${CMAKE_LIBRARY_PATH}/include")

add_dependencies(blare_pcre2_lib pcre2)

# target_link_libraries(blare_pcre2_lib ${PCRE2_TARGET}/lib)
# find_package(pcre2 CONFIG COMPONENTS 8BIT 16BIT 32BIT)
# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/pcre2
# find_library(NAMES pcre2-8 pcre2-16 pcre2-32)

target_link_libraries(blare_pcre2_lib PRIVATE misc_lib Boost::boost Threads::Threads -L{$PCRE2_TARGET}/lib -lpcre2-8)

# find_package(pcre2 COMPONENTS pcre2-16)
# if (${pcre2-16_FOUND})
#     target_link_libraries(blare_pcre2_lib PRIVATE -lpcre2-16)
# endif(${pcre2-16_FOUND})
# find_package(pcre2 COMPONENTS pcre2-32)
# if (${pcre2-32_FOUND})
#     target_link_libraries(blare_pcre2_lib PRIVATE -lpcre2-32)
# endif(${pcre2-32_FOUND})

target_compile_features(blare_pcre2_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_pcre2"
  PREFIX "BLARE PCRE2 Header Files"
  FILES ${HEADER_LIST})