from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, cmake_layout
import os


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    test_type = "explicit"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires(self.tested_reference_str)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_core")
            self.run(bin_path, env="conanrun")
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_utest")
            self.run(bin_path, env="conanrun")
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_grpc")
            self.run(bin_path, env="conanrun")
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_mongo")
            self.run(bin_path, env="conanrun")
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_postgresql")
            self.run(bin_path, env="conanrun")
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_rabbitmq")
            self.run(bin_path, env="conanrun")
            bin_path = os.path.join(self.cpp.build.bindirs[0], "PackageTest_redis")
            self.run(bin_path, env="conanrun")

            bin_path = os.path.join(
                self.cpp.build.bindirs[0], 
                "testsuite-support", 
                "runtests-testsuite-userver-samples-testsuite-support")
            command = " "
            folder = os.path.join(self.recipe_folder, "..", "..", "samples", "testsuite-support", "tests")
            args = [bin_path, "--service-logs-pretty", "-vv", folder]
            self.run(command.join(args), env="conanrun")

