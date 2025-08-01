from conan import ConanFile
from conan.tools.cmake import cmake_layout


class CppGraphIndexConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "MesonDeps", "PkgConfigDeps"
    
    def requirements(self):
        # Dependencies will be added in later steps
        pass
    
    def build_requirements(self):
        # Build-only dependencies will be added as needed
        pass
    
    def layout(self):
        cmake_layout(self)