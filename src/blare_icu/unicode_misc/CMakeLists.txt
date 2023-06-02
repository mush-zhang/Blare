file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_icu/unicode_misc/*.hpp")

add_library(unicode_misc_lib EXCLUDE_FROM_ALL split_regex.cpp ${HEADER_LIST})

target_include_directories(unicode_misc_lib PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(unicode_misc_lib PUBLIC ${ICU_LIBRARIES})

target_compile_features(unicode_misc_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_icu/unicode_misc"
  PREFIX "BLARE ICU Unicode Misc Header Files"
  FILES ${HEADER_LIST})
