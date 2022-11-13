import conans  # pylint: disable=import-error


class UserverTestConan(conans.ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'cmake_find_package'

    def requirements(self):
        self.requires('userver/1.0.0')

    def build(self):
        cmake = conans.CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        self.run('./hello_service')
