cmake_minimum_required(VERSION 3.20.0)

project(llvm-project-dummy)

# Simply delegate to the llvm subdirectory
# This preserves the original build logic while providing
# a CMakeLists.txt in the project root
add_subdirectory(llvm)
