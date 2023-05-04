# Copyright 2022 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

set_and_check(re2_INCLUDE_DIR ${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_INCLUDEDIR@)

if(UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_dependency(Threads REQUIRED)
endif()

check_required_components(re2)

if(TARGET re2::re2)
  return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/re2Targets.cmake)