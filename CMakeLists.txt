cmake_minimum_required(VERSION 3.12)

project(
  BLARE
  VERSION 0.1
  DESCRIPTION "BLARE: a regular expression matching framework"
  LANGUAGES CXX)

set(CMAKE_LIBRARY_PATH "/usr/local/lib")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
 set(CMAKE_CXX_EXTENSIONS OFF)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -pthread")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -pthread")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -pthread")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast -march=native -mfma -mavx -fomit-frame-pointer \
    -ffp-contract=fast -flto -DARMA_NO_DEBUG -pthread")

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
find_package(Boost REQUIRED)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 5.3.0)
FetchContent_MakeAvailable(fmtlib)

add_subdirectory(src)
add_subdirectory(experiments)

if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND BUILD_TESTING)
  add_subdirectory(tests)
endif()