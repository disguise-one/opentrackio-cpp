from conan import ConanFile
from conans import ConanFile, CMake, tools
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd

# OpenTrackIO-cpp build file.
# This has been modified to support disguises
# conan packaging system.

class opentrackiocppRecipe(ConanFile):
    name = "opentrackio-cpp"
    version = "1.0.1.1"
    package_type = "library"

    license = "MIT"
    author = "Mo-Sys Engineering Ltd"
    url = "https://github.com/mosys/opentrackio-cpp"
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
    
    def build(self):
         # Ideally this should be determined by the conan build settings, however our internal conan packaging system only
         # operates with the `Release` package type. So we need to build for both debug and release and then source the 
         # correct package as needed.
         # Adapted from Disguise CEF build recipe.
         for build_type in ["Debug", "Release"]:
            # Override global build type value. Used by conan code to do stuff and configure cmake correctly.
            self.settings.build_type = build_type 
            cmake = CMake(self, generator="Visual Studio 17 2022", build_type=build_type)
            cmake.configure()
            cmake.build()

    # Taken from Disguise CEF build.
    def package_id(self):
        # Normally we wouldn't overload this function. This function overrides the uniqueid of the package, which would normally
        # be different depending on compiler version, arch and build type. Thes aren't valud for d3 conan install, so just
        # use header_only flag, which means its always the same regardless
        self.info.header_only()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        self.copy("*", src="include", dst="include", keep_path=True)
        self.copy("*", src="Debug", dst="lib\\Debug", keep_path=True)
        self.copy("*", src="Release", dst="lib\\Release", keep_path=True)
    
    def package_info(self):
        self.cpp_info.libs = ['opentrackio-cpp']