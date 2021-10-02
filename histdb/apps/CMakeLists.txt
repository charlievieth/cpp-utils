add_executable(histdb main.cc)

target_compile_features(app PRIVATE cxx_std_17)

# target_link_libraries(app PRIVATE modern_library fmt::fmt)
