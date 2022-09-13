from conans import ConanFile, CMake, tools
import os


class UserverTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"

    def requirements(self):
        self.requires("userver/1.0.0")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self):
            bin_path = os.path.join("bin", "example")
            self.run(bin_path, run_environment=True)
