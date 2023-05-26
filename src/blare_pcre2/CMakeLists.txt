find_package(pcre2 COMPONENTS pcre2-8)

if(NOT ${pcre2_FOUND})
    # rerun cmake in initial build
    # will update cmakecache/project files on first build
    # so you may have to reload project after first build
    add_custom_target(Rescan ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} DEPENDS pcre2)
else()
    # Rescan becomes a dummy target after first build
    # this prevents cmake from rebuilding cache/projects on subsequent builds
    add_custom_target(Rescan)
endif()

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
    ${CMAKE_BINARY_DIR}/_deps/include/jpcre2.hpp)

target_include_directories(blare_pcre2_lib 
    PUBLIC 
    "/usr/local/include" 
    "${PROJECT_BINARY_DIR}" 
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/_deps/include"
    "${CMAKE_LIBRARY_PATH}/include")

add_dependencies(blare_pcre2_lib Rescan)
if(${pcre2_FOUND})
    target_link_libraries(testapp ${pcre2_LIBRARY})
endif()
target_link_libraries(blare_pcre2_lib PRIVATE misc_lib Boost::boost Threads::Threads -lpcre2-8)

find_package(pcre2 COMPONENTS pcre2-16)
if (${pcre2-16_FOUND})
    target_link_libraries(blare_pcre2_lib PRIVATE -lpcre2-16)
endif(${pcre2-16_FOUND})
find_package(pcre2 COMPONENTS pcre2-32)
if (${pcre2-32_FOUND})
    target_link_libraries(blare_pcre2_lib PRIVATE -lpcre2-32)
endif(${pcre2-32_FOUND})

target_compile_features(blare_pcre2_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_pcre2"
  PREFIX "BLARE PCRE2 Header Files"
  FILES ${HEADER_LIST})