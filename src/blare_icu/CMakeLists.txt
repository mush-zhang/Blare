# ICU Components Reference:
# https://cmake.org/cmake/help/latest/module/FindICU.html
# ICU components = data, i18n, io, le, lx, test, tu and uc.

# find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)

# Link using the imported target names (ICU::uc, ICU::i18n, etc.)

# target_link_libraries(myapp ICU::uc ICU::i18n ICU::io)

# find_package(ICU REQUIRED COMPONENTS regex) 

set(ICU_MAJOR_VER "72")
set(ICU_FULL_VERSION "${ICU_MAJOR_VER}.1")

find_library(ICUIL18N_LIB libicui18n.so.${ICU_FULL_VERSION} PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ NO_DEFAULT_PATH)
find_library(ICUUC_LIB libicuuc.so.${ICU_FULL_VERSION} PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ NO_DEFAULT_PATH)
find_library(ICUDATA_LIB libicudata.so.${ICU_FULL_VERSION} PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ NO_DEFAULT_PATH)

find_library(ICUIL18N_MAJOR_LIB libicui18n.so.${ICU_MAJOR_VER} PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ NO_DEFAULT_PATH)
find_library(ICUUC_MAJOR_LIB libicuuc.so.${ICU_MAJOR_VER} PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ NO_DEFAULT_PATH)
find_library(ICUDATA_MAJOR_LIB libicudata.so.${ICU_MAJOR_VER} PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ NO_DEFAULT_PATH)

install(FILES ${ICUIL18N_LIB} ${ICUUC_LIB} ${ICUDATA_LIB}
              ${ICUIL18N_MAJOR_LIB} ${ICUUC_MAJOR_LIB} ${ICUDATA_MAJOR_LIB}
        DESTINATION lib)


# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${BLARE_SOURCE_DIR}/include/blare_icu/*.hpp")
# set(HEADER_LIST "${BLARE_SOURCE_DIR}/include/blare_icu/lib.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(blare_icu_lib 
    MODULE
    blare.cpp
    split_match_3way.cpp 
    split_match_multiway.cpp 
    ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(blare_icu_lib PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

# This depends on (header only) icu
target_link_libraries(blare_icu_lib PRIVATE unicode_misc_lib  ICU::uc ICU::i18n ICU::io) 


# All users of this library will need at least C++17
target_compile_features(blare_icu_lib PUBLIC cxx_std_17)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include/blare_icu"
  PREFIX "BLARE ICU Header Files"
  FILES ${HEADER_LIST})