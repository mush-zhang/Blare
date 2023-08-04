cmake_minimum_required(VERSION 3.14)

project(
  BLARE
  VERSION 0.1
  DESCRIPTION "BLARE: a regular expression matching framework"
  LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED True)

  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  find_package(Doxygen)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
endif()

include(FetchContent)

find_package(Boost REQUIRED COMPONENTS regex) 

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG 2022-06-01
  GIT_SHALLOW TRUE
)
if (EXISTS BUILD_SHARED_LIBS)
  set(ORIGIN_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
endif()
option(BUILD_SHARED_LIBS "build shared libraries" ON)
FetchContent_MakeAvailable(re2)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/_deps/re2-build")
set(BUILD_SHARED_LIBS ORIGIN_BUILD_SHARED_LIBS)

set(PCRE2_PROJECT pcre2)
set(PCRE_PREFIX ${CMAKE_BINARY_DIR}/_deps/pcre2)
set(PCRE_SRC ${PCRE_PREFIX}/src/pcre2)
set(PCRE2_TARGET ${CMAKE_BINARY_DIR}/_deps/pcre2-build)
file(MAKE_DIRECTORY ${PCRE2_TARGET})
set(PCRE2_LIB_DIR ${PCRE2_TARGET}/lib)
file(MAKE_DIRECTORY ${PCRE2_LIB_DIR})
set(PCRE2_INC_DIR ${PCRE2_TARGET}/include)
file(MAKE_DIRECTORY ${PCRE2_INC_DIR})

add_library (pcre2-8 SHARED IMPORTED)
set_target_properties (pcre2-8 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIB_DIR}/libpcre2-8.so)
target_include_directories(pcre2-8 INTERFACE ${PCRE2_INC_DIR})

add_library (pcre2-16 SHARED IMPORTED)
set_target_properties (pcre2-16 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIB_DIR}/libpcre2-16.so)
target_include_directories(pcre2-16 INTERFACE ${PCRE2_INC_DIR})

add_library (pcre2-32 SHARED IMPORTED)
set_target_properties (pcre2-32 PROPERTIES IMPORTED_LOCATION ${PCRE2_LIB_DIR}/libpcre2-32.so)
target_include_directories(pcre2-32 INTERFACE ${PCRE2_INC_DIR})

set(PCRE2_LIBRARIES pcre2-8 pcre2-16 pcre2-32)

file(DOWNLOAD
  https://raw.githubusercontent.com/jpcre2/jpcre2/10.32.01/src/jpcre2.hpp
  ${CMAKE_BINARY_DIR}/_deps/include/jpcre2.hpp
)

set(ICU_PROJECT ICU)
set(ICU_PREFIX ${CMAKE_BINARY_DIR}/_deps/ICU)
set(ICU_SRC ${ICU_PREFIX}/src/ICU/source)
set(ICU_TARGET ${CMAKE_BINARY_DIR}/_deps/ICU-build)
file(MAKE_DIRECTORY ${ICU_TARGET})
set(ICU_LIB_DIR ${ICU_TARGET}/lib)
file(MAKE_DIRECTORY ${ICU_LIB_DIR})
set(ICU_INC_DIR ${ICU_TARGET}/include)
file(MAKE_DIRECTORY ${ICU_INC_DIR})

add_library(ICU::uc SHARED IMPORTED)
set_target_properties (ICU::uc PROPERTIES IMPORTED_LOCATION ${ICU_LIB_DIR}/libicuuc.so)
target_include_directories(ICU::uc INTERFACE ${ICU_INC_DIR})

add_library(ICU::i18n SHARED IMPORTED)
set_target_properties (ICU::i18n PROPERTIES IMPORTED_LOCATION ${ICU_LIB_DIR}/libicui18n.so)
target_include_directories(ICU::i18n INTERFACE ${ICU_INC_DIR})

add_library(ICU::io SHARED IMPORTED)
set_target_properties (ICU::io PROPERTIES IMPORTED_LOCATION ${ICU_LIB_DIR}/libicuio.so)
target_include_directories(ICU::io INTERFACE ${ICU_INC_DIR})

add_library(ICU::data SHARED IMPORTED)
set_target_properties (ICU::data PROPERTIES IMPORTED_LOCATION ${ICU_LIB_DIR}/libicudata.so)
target_include_directories(ICU::data INTERFACE ${ICU_INC_DIR})

set(ICU_LIBRARIES ICU::uc ICU::i18n ICU::io ICU::data)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -pthread")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -pthread")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
  -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-O3 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
  -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread")
  
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  set(DATA_DIR ${CMAKE_BINARY_DIR}/data)
  file(CREATE_LINK ${CMAKE_SOURCE_DIR}/data/ ${DATA_DIR} SYMBOLIC)
endif()

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(experiments)
add_subdirectory(original_codebase)