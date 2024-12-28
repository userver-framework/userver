### This conanfile.py can be placed into a sample directory to
### use that sample as a test for userver package build with conan

from conan import ConanFile
from conan.tools import build
from conan.tools import cmake as tool_cmake


class TestUserverPackageConan(ConanFile):
    settings = 'os', 'arch', 'compiler', 'build_type'
    generators = 'CMakeToolchain', 'CMakeDeps'

    def layout(self):
        tool_cmake.cmake_layout(self)

    def requirements(self):
        self.requires(self.tested_reference_str)

    def build(self):
        cmake = tool_cmake.CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if build.can_run(self):
            self.run('ctest -V')
