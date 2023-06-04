file(GLOB HEADER_LIST CONFIGURE_DEPENDS 
    "/usr/local/include" 
    "${PROJECT_BINARY_DIR}" 
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/_deps/include"
    "${CMAKE_LIBRARY_PATH}/include")

add_executable(blare_re2 blare_re2.cpp)
target_include_directories(blare_re2 PUBLIC PUBLIC HEADER_LIST)
target_compile_features(blare_re2 PRIVATE cxx_std_17)
target_link_libraries(blare_re2 PRIVATE Boost::boost Threads::Threads re2::re2 Threads::Threads)

add_executable(blare_pcre2 blare_pcre2.cpp)
target_include_directories(blare_pcre2 PUBLIC HEADER_LIST)
target_compile_features(blare_pcre2 PRIVATE cxx_std_17)
target_link_libraries(blare_pcre2 PRIVATE Boost::boost Threads::Threads ${PCRE2_LIBRARIES})

add_executable(blare_boost blare_boost.cpp)
target_include_directories(blare_boost PUBLIC HEADER_LIST)
target_compile_features(blare_boost PRIVATE cxx_std_17)
target_link_libraries(blare_boost PRIVATE Boost::boost Boost::regex)

add_executable(blare_icu blare_icu.cpp)
target_include_directories(blare_icu PUBLIC HEADER_LIST)
target_compile_features(blare_icu PRIVATE cxx_std_17)
target_link_libraries(blare_icu PRIVATE ${ICU_LIBRARIES})
