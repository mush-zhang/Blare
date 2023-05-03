add_executable(blare main.cpp)

target_compile_features(blare PRIVATE cxx_std_17)

target_link_libraries(blare PRIVATE blare_re2 )