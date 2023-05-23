file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/misc/*.hpp")

add_library(misc_lib split_regex.cpp ${HEADER_LIST})

target_include_directories(misc_lib PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_compile_features(misc_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/misc"
  PREFIX "BLARE Misc Header Files"
  FILES ${HEADER_LIST})
