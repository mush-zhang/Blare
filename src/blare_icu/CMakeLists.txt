# find_package(ICU 72 COMPONENTS uc i18n io REQUIRED)

# set(ICU_MAJOR_VER "72")
# set(ICU_FULL_VERSION "${ICU_MAJOR_VER}.1")

add_subdirectory(unicode_misc)

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_icu/*.hpp")

add_library(blare_icu_lib 
    EXCLUDE_FROM_ALL
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST})

target_include_directories(blare_icu_lib PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(blare_icu_lib PRIVATE unicode_misc_lib  ICU::uc ICU::i18n ICU::io) 

target_compile_features(blare_icu_lib PUBLIC cxx_std_17)

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_icu"
  PREFIX "BLARE ICU Header Files"
  FILES ${HEADER_LIST})
