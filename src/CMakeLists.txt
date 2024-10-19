add_executable(svf-taint svf-taint.cpp)
target_link_libraries(svf-taint ${llvm_libs} ${SVF_LIB})

# add_library(TaintTracker . )
