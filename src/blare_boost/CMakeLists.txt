find_package(Boost REQUIRED COMPONENTS regex) 
# If you installed the library at a custom location, remember to add that custom location in your CMake prefixes:
# cmake .. -DCMAKE_PREFIX_PATH=/path/to/custom/location/of/boost
find_library(NAMES boost_regex)

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_boost/*.hpp")

add_library(blare_boost_lib 
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST})

target_include_directories(blare_boost_lib PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(blare_boost_lib PRIVATE misc_lib Boost::boost Boost::regex) 

target_compile_features(blare_boost_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_boost"
  PREFIX "BLARE Boost Header Files"
  FILES ${HEADER_LIST})
