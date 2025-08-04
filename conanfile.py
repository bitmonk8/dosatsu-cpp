from conan import ConanFile


class CppGraphIndexConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "MesonToolchain", "PkgConfigDeps", "CMakeDeps"
    
    # Performance optimizations
    default_options = {
        # Enable fast dependency resolution
        "llvm-core/*:shared": False,  # Static linking for better performance
    }
    
    # Configure revision modes for better caching
    revision_mode = "scm"
    
    def requirements(self):
        # Core LLVM library for C++ AST parsing and analysis
        self.requires("llvm-core/19.1.7")
        # Testing framework for unit tests
        self.requires("doctest/2.4.11")
        # Compression libraries required by LLVM
        self.requires("zlib/1.3.1")
        self.requires("zstd/1.5.6")
    
    def build_requirements(self):
        # Build-only dependencies will be added as needed
        pass
    
    def layout(self):
        # Configure generators folder to avoid cluttering root  
        self.folders.generators = "conan"
        # Optimize build folders for better caching
        self.folders.build = "conan/build"
        self.folders.cache = "conan/cache"
    
    def configure(self):
        # Performance-oriented dependency configuration
        # Enable parallel dependency installation when possible
        if self.settings.os == "Linux" and self.settings.compiler == "gcc":
            self.settings.compiler.libcxx = "libstdc++11"