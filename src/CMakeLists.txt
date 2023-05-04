add_executable(blare main.cpp)

target_compile_features(blare PRIVATE cxx_std_17)

target_link_libraries(blare PRIVATE blare_re2_lib blare_pcre2_lib blare_boost_lib)