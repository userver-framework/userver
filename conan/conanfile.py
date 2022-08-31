from conans import ConanFile, CMake, tools
import os

required_conan_version = ">=1.47.0"

class UserverConan(ConanFile):
    name = "userver"
    version = "1.0.0"
    description = "The C++ Asynchronous Framework"
    topics = ("framework", "coroutines", "asynchronous")
    url = "https://github.com/userver-framework/userver"
    homepage = "https://userver.tech/"
    license = "Apache-2.0"
    generators = "cmake"

    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    scm = {
        "type": "git",
        "url": "https://github.com/userver-framework/userver.git",
        "revision": "develop"
    }

    def configure(self):
        if self.options.shared:
            del self.options.fPIC

    def requirements(self):
        self.requires("boost/1.79.0")
        self.requires("libev/4.33")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_TESTS"] = False  # example
        cmake.configure(build_folder="cmake-build")
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        cmake = self._configure_cmake()
        cmake.install()
        # If the CMakeLists.txt has a proper install method, the steps below may be redundant
        # If so, you can just remove the lines below
        include_folder = os.path.join(self._source_subfolder, "include")
        self.copy(pattern="*", dst="include", src=include_folder)
        self.copy(pattern="*.dll", dst="bin", keep_path=False)
        self.copy(pattern="*.lib", dst="lib", keep_path=False)
        self.copy(pattern="*.a", dst="lib", keep_path=False)
        self.copy(pattern="*.so*", dst="lib", keep_path=False)
        self.copy(pattern="*.dylib", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)