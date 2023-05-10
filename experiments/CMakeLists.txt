add_executable(re2_expr re2_expr.cpp)

target_include_directories(re2_expr PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

target_compile_features(re2_expr PRIVATE cxx_std_17)

target_link_libraries(re2_expr PRIVATE blare_re2_lib -pthread)