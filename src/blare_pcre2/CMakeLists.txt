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

target_link_libraries(blare_pcre2_lib PUBLIC misc_lib Boost::boost Threads::Threads ${PCRE2_LIBRARIES})

target_compile_features(blare_pcre2_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_pcre2"
  PREFIX "BLARE PCRE2 Header Files"
  FILES ${HEADER_LIST})