cmake_minimum_required(VERSION 3.12)

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

  # Testing only available if this is the main app
  # Note this needs to be done in the main CMakeLists
  # since it calls enable_testing, which must be in the
  # main CMakeLists.
  # include(CTest)

  find_package(Doxygen)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
endif()

include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.65.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Boost)

FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG 2022-06-01
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(re2)

set(PCRE_PREFIX ${CMAKE_BINARY_DIR}/_deps/pcre2)
set(PCRE_SRC ${PCRE_PREFIX}/src/pcre2)
set(PCRE2_TARGET ${CMAKE_BINARY_DIR}/_deps/pcre2-build)
ExternalProject_Add(
  pcre2
  GIT_REPOSITORY https://github.com/PCRE2Project/pcre2
  GIT_TAG pcre2-10.40
  GIT_SHALLOW TRUE
  PREFIX ${PCRE_PREFIX}
  DOWNLOAD_COMMAND git clone --branch pcre2-10.40 --depth 1 https://github.com/PCRE2Project/pcre2
  && cd ${PCRE_SRC} && ${PCRE_SRC}/autogen.sh
  CONFIGURE_COMMAND ${PCRE_SRC}/configure --srcdir=${PCRE_SRC} --prefix=${PCRE2_TARGET} --enable-jit --enable-pcre2-16 --enable-pcre2-32  --disable-static
  BUILD_COMMAND make
  TEST_COMMAND make check
  INSTALL_COMMAND sudo make install
)

set(CMAKE_LIBRARY_PATH /usr/local/lib ${PCRE2_TARGET}/lib)

FetchContent_Declare(
  ICU
  URL https://github.com/unicode-org/icu/releases/download/release-72-1/icu4c-72_1-src.tgz
  URL_HASH MD5=4de6484a6c460d3d1468a6fe50369439
)
FetchContent_MakeAvailable(ICU)

file(DOWNLOAD
  https://raw.githubusercontent.com/jpcre2/jpcre2/10.32.01/src/jpcre2.hpp
  ${CMAKE_BINARY_DIR}/_deps/include/jpcre2.hpp
)

find_package(Boost REQUIRED)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -pthread")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -pthread")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
  -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-O3 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
  -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread")

add_subdirectory(src)
add_subdirectory(experiments)

# if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND BUILD_TESTING)
#   add_subdirectory(tests)
# endif()