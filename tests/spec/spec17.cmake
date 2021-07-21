add_compile_definitions(SPEC NDEBUG SPEC_LP64 SPEC_SUPPRESS_OPENMP)
add_compile_options(-O3 -march=haswell -fno-unsafe-math-optimizations -fno-tree-loop-vectorize -DSPEC_SUPPRESS_OPENMP)
