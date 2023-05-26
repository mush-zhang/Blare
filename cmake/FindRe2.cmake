include(FindPackageHandleStandardArgs)

if(NOT re2_NO_re2_CMAKE)

  find_package(re2 QUIET NO_MODULE)

  if(re2_FOUND)
    find_package_handle_standard_args(re2 CONFIG_MODE)
    return()
  endif()
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(re2 QUIET re2 IMPORTED_TARGET GLOBAL)
  if(re2_FOUND)
    add_library(re2::re2 ALIAS PkgConfig::re2)
  endif()
endif()