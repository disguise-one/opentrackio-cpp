from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

# OpenTrackIO-cpp build file.
# This has been modified to support disguises
# conan packaging system.

class opentrackiocppRecipe(ConanFile):
    name = "opentrackio-cpp"
    version = "1.0.1"
    package_type = "library"

    license = "MIT"
    author = "Mo-Sys Engineering Ltd"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "A Cpp helper library for usage with the OpenTrackIO protocol."
    topics = ("OpenTrackIO")

    settings = {"os", "compiler", "build_type", "arch"}
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    exports_sources = "CMakeLists.txt", "src/*", "include/*", "external/*", "cmake/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")
        
        self.settings.compiler.cppstd = 20

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
            
    def validate(self):
        if self.settings.arch != "x86_64":
            raise ConanInvalidConfiguration("Only x86_64 supported in this package!")
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("Only Windows builds supported in this package!")
        
        check_min_cppstd(self, 20)
    
    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generator = "Visual Studio 17 2022"
        tc.variables["BUILD_STATIC_LIBS"] = "ON"
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
    
    def package_info(self):
        self.cpp_info.libs = ['opentrackio-cpp']