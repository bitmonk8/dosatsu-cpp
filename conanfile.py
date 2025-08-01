from conan import ConanFile
from conan.tools.cmake import cmake_layout


class CppGraphIndexConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "MesonToolchain", "PkgConfigDeps"
    
    def requirements(self):
        # Core LLVM library for C++ AST parsing and analysis
        self.requires("llvm-core/19.1.7")
        # Testing framework for unit tests
        self.requires("doctest/2.4.11")
    
    def build_requirements(self):
        # Build-only dependencies will be added as needed
        pass
    
    def layout(self):
        cmake_layout(self)