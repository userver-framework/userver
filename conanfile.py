# pylint: disable=no-member

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import cross_building
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import apply_conandata_patches, collect_libs, copy, export_conandata_patches, get, rename, replace_in_file, rmdir, save
from conan.tools.microsoft import is_msvc, is_msvc_static_runtime

import os

required_conan_version = '>=1.51.0, <2.0.0'  # pylint: disable=invalid-name


class UserverConan(ConanFile):
    name = 'userver'
    version = '1.0.0'
    description = 'The C++ Asynchronous Framework'
    topics = ('framework', 'coroutines', 'asynchronous')
    url = 'https://github.com/userver-framework/userver'
    homepage = 'https://userver.tech/'
    license = 'Apache-2.0'
    exports_sources = '*'

    settings = 'os', 'arch', 'compiler', 'build_type'
    options = {
        'shared': [True, False],
        'fPIC': [True, False],
        'lto': [True, False],
        'with_jemalloc': [True, False],
        'with_mongodb': [True, False],
        'with_postgresql': [True, False],
        'with_postgresql_extra': [True, False],
        'with_redis': [True, False],
        'with_grpc': [True, False],
        'with_clickhouse': [True, False],
        'with_universal': [True, False],
        'with_rabbitmq': [True, False],
        'with_utest': [True, False],
        'namespace': ['ANY'],
        'namespace_begin': ['ANY'],
        'namespace_end': ['ANY'],
    }

    default_options = {
        'shared': False,
        'fPIC': True,
        'lto': True,
        'with_jemalloc': True,
        'with_mongodb': True,
        'with_postgresql': True,
        'with_postgresql_extra': False,
        'with_redis': True,
        'with_grpc': True,
        'with_clickhouse': False,
        'with_universal': True,
        'with_rabbitmq': True,
        'with_utest': True,
        'namespace': 'userver',
        'namespace_begin': '',
        'namespace_end': '',
    }

    # scm = {
    #     'type': 'git',
    #     'url': 'https://github.com/userver-framework/userver.git',
    #     'revision': 'develop'
    # }

    @property
    def _source_subfolder(self):
        return 'source'

    @property
    def _build_subfolder(self):
        return os.path.join(self.build_folder, 'userver')

    def configure(self):
        if self.options.shared:
            del self.options.fPIC

        if not self.options.namespace_begin:
            self.options.namespace_begin = (
                f'namespace {self.options.namespace} {{'
            )
        if not self.options.namespace_end:
            self.options.namespace_end = '}'


    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires('boost/1.79.0')
        self.requires('c-ares/1.18.1')
        self.requires('cctz/2.3')
        self.requires('concurrentqueue/1.0.3')
        self.requires('cryptopp/8.6.0')
        self.requires('fmt/8.1.1')
        self.requires('http_parser/2.9.4')
        self.requires('libcurl/7.86.0')
        self.requires('libev/4.33')
        self.requires('libnghttp2/1.51.0')
        self.requires('openssl/1.1.1s')
        self.requires('rapidjson/cci.20220822')
        self.requires('spdlog/1.9.0')
        self.options['spdlog'].header_only = True
        self.requires('yaml-cpp/0.7.0')
        self.requires('zlib/1.2.13')

        if self.options.with_jemalloc:
            self.requires('jemalloc/5.2.1')
        if self.options.with_grpc:
            self.requires('grpc/1.48.0')
        if self.options.with_postgresql:
            self.requires('libpq/14.5')
        if self.options.with_mongodb:
            self.requires('mongo-c-driver/1.22.0')
            self.options['mongo-c-driver'].with_sasl = 'cyrus'
        if self.options.with_redis:
            self.requires('hiredis/1.0.2')
        if self.options.with_rabbitmq:
            self.requires('amqp-cpp/4.3.16')
        if self.options.with_utest:
            self.requires('gtest/1.12.1')
            self.requires('benchmark/1.6.2')



    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables['CMAKE_FIND_DEBUG_MODE'] = False

        tc.variables['USERVER_OPEN_SOURCE_BUILD'] = True
        tc.variables['USERVER_CONAN'] = True
        tc.variables['USERVER_IS_THE_ROOT_PROJECT'] = False
        tc.variables['USERVER_DOWNLOAD_PACKAGES'] = True
        tc.variables['USERVER_FEATURE_DWCAS'] = True
        tc.variables['USERVER_NAMESPACE'] = self.options.namespace
        tc.variables[
            'USERVER_NAMESPACE_BEGIN'
        ] = self.options.namespace_begin
        tc.variables['USERVER_NAMESPACE_END'] = self.options.namespace_end

        tc,variables['USERVER_LTO'] = self.options.lto
        tc.variables['USERVER_FEATURE_JEMALLOC'] = self.options.with_jemalloc
        tc.variables['USERVER_FEATURE_MONGODB'] = self.options.with_mongodb
        tc.variables['USERVER_FEATURE_POSTGRESQL'] = self.options.with_postgresql
        tc.variables['USERVER_FEATURE_PATCH_LIBPQ'] = self.options.with_postgresql_extra
        tc.variables['USERVER_FEATURE_REDIS'] = self.options.with_redis
        tc.variables['USERVER_FEATURE_GRPC'] = self.options.with_grpc
        tc.variables['USERVER_FEATURE_CLICKHOUSE'] = self.options.with_clickhouse
        tc.variables['USERVER_FEATURE_UNIVERSAL'] = self.options.with_universal
        tc.variables['USERVER_FEATURE_RABBITMQ'] = self.options.with_rabbitmq
        tc.variables['USERVER_FEATURE_UTEST'] = self.options.with_utest

        tc.generate()

        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy(pattern='LICENSE', dst='licenses')

        self.copy(
            pattern='*', dst='include', src='core/include', keep_path=True,
        )
        self.copy(
            pattern='*', dst='include', src='shared/include', keep_path=True,
        )
        print(self._build_subfolder)

        if self.options.with_universal:
            copy(self, pattern='*', dst='include', src='universal/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'universal'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'universal'), keep_path=False)
        if self.options.with_grpc:
            copy(self, pattern='*', dst='include', src='grpc/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'grpc'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'grpc'), keep_path=False)
        if self.options.with_utest:
            self.copy(
                pattern='*',
                dst='include',
                src='core/testing/include',
                keep_path=True,
            )
            self.copy(
                pattern='*', dst='testsuite', src='testsuite', keep_path=True,
            )
            self.copy(
                pattern='UserverTestsuite.cmake',
                dst='cmake',
                src='cmake',
                keep_path=True,
            )
            self.copy(
                pattern='AddGoogleTests.cmake',
                dst='cmake',
                src='cmake',
                keep_path=True,
            )
        if self.options.with_postgresql:
            copy(self, pattern='*', dst='include', src='postgresql/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'postgresql'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'postgresql'), keep_path=False)

        if self.options.with_mongodb:
            copy(self, pattern='*', dst='include', src='mongo/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'mongo'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'mongo'), keep_path=False)
        if self.options.with_redis:
            copy(self, pattern='*', dst='include', src='redis/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'redis'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'redis'), keep_path=False)
        if self.options.with_rabbitmq:
            copy(self, pattern='*', dst='include', src='rabbitmq/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'rabbitmq'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'rabbitmq'), keep_path=False)
        if self.options.with_clickhouse:
            copy(self, pattern='*', dst='include', src='clickhouse/include', keep_path=True)
            copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'clickhouse'), keep_path=False)
            copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'clickhouse'), keep_path=False)

        copy(self, pattern='*.a', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder,'core'), keep_path=False)
        copy(self, pattern='*.so', dst=os.path.join(self.package_folder, "lib"), src=os.path.join(self._build_subfolder, 'core'), keep_path=False)

    @property
    def _userver_components(self):

        def grpc():
            return ["grpc::grpc"] if self.options.with_grpc else []

        def fmt():
            return ["fmt::fmt"]
        def cctz():
            return ["cctz::cctz"]
        def boost():
            return ["boost::boost"]
        def concurrentqueue():
            return ["concurrentqueue::concurrentqueue"]
        def yaml():
            return ["yaml-cpp::yaml-cpp"]
        def libev():
            return ["libev::libev"]
        def http_parser():
            return ["http_parser::http_parser"]    
        def grpc():
            return ["grpc::grpc"] if self.options.with_grpc else []
        def postgresql():
            return ["libpq::pq"] if self.options.with_postgresql else []
        def gtest():
            return ["gtest::gtest"] if self.options.with_utest else []
        def benchmark():
            return ["benchmark::benchmark"] if self.options.with_utest else []            
        def mongo():
            return ["mongo::mongoc_static"] if self.options.with_mongodb else []                                    
        def hiredis():
            return ["hiredis::hiredis"] if self.options.with_redis else []
        def amqpcpp():
            return ["amqpcpp"] if self.options.with_rabbitmq else []  

        userver_components = [
            {"target": "userver-core-internal",       "lib": "core-internal",       "requires": [] },
            {"target": "userver-core",       "lib": "core",       "requires": ["userver-core-internal"] + fmt() + cctz() + boost() + concurrentqueue() + yaml() + libev() + http_parser() }
        ]
        if self.options.with_universal:
            userver_components.extend([
                {"target": "userver-universal", "lib": "universal", "requires": ["userver-core"] }
            ])        
        if self.options.with_grpc:
            userver_components.extend([
                {"target": "userver-grpc", "lib": "grpc", "requires": ["userver-core"] + grpc() },
                {"target": "userver-api-common-protos",      "lib": "api-common-protos",      "requires": ["userver-grpc"] }
            ])
        if self.options.with_utest:
            userver_components.extend([
                {"target": "userver-utest", "lib": "utest", "requires": ["userver-core"] + gtest() },
                {"target": "userver-ubench", "lib": "ubench", "requires": ["userver-core"] + benchmark() }
            ])
        if self.options.with_postgresql:
            userver_components.extend([
                {"target": "userver-postgresql", "lib": "postgresql", "requires": ["userver-core"] + postgresql() }
            ])
        # if self.options.with_mongodb:
        #     userver_components.extend([
        #         {"target": "userver-mongo", "lib": "mongo", "requires": ["userver-core"] + mongo() }
        #     ])
        if self.options.with_redis:
            userver_components.extend([
                {"target": "userver-redis", "lib": "redis", "requires": ["userver-core"] + hiredis() }
            ])       
        # if self.options.with_rabbitmq:
        #     userver_components.extend([
        #         {"target": "userver-rabbitmq", "lib": "rabbitmq", "requires": ["userver-core"] + amqpcpp() }
        #     ])                                            
        return userver_components 


    def package_info(self):
        print(self.folders)
      #  self.cpp_info.libs = conans.tools.collect_libs(self)
        debug = "d" if self.settings.build_type == "Debug" and self.settings.os == "Windows" else ""

        def get_lib_name(module):
            return f"userver-{module}{debug}"

        def add_components(components):
            for component in components:
                conan_component = component["target"]
                cmake_target = component["target"]
                cmake_component = component["lib"]
                lib_name = get_lib_name(component["lib"])
                requires = component["requires"]
                # TODO: we should also define COMPONENTS names of each target for find_package() but not possible yet in CMakeDeps
                #       see https://github.com/conan-io/conan/issues/10258
                self.cpp_info.components[conan_component].set_property("cmake_target_name", cmake_target)
                self.cpp_info.components[conan_component].libs = [lib_name]
                if self.settings.os == "Linux":
                    self.cpp_info.components[conan_component].system_libs = ["pthread"]

                # TODO: to remove in conan v2 once cmake_find_package* generators removed
                self.cpp_info.components[conan_component].names["cmake_find_package"] = cmake_target
                self.cpp_info.components[conan_component].names["cmake_find_package_multi"] = cmake_target
                print(self.cpp_info.components[conan_component].requires)
                print(requires)
                self.cpp_info.components[conan_component].requires = requires
             #   self.cpp_info.components[conan_component].build_modules["cmake_find_package"] = [self._module_file_rel_path]
             #   self.cpp_info.components[conan_component].build_modules["cmake_find_package_multi"] = [self._module_file_rel_path]
                if cmake_component != cmake_target:
                    conan_component_alias = conan_component + "_alias"
                    self.cpp_info.components[conan_component_alias].names["cmake_find_package"] = cmake_component
                    self.cpp_info.components[conan_component_alias].names["cmake_find_package_multi"] = cmake_component
                    self.cpp_info.components[conan_component_alias].requires = [conan_component]
                    self.cpp_info.components[conan_component_alias].bindirs = []
                    self.cpp_info.components[conan_component_alias].includedirs = []
                    self.cpp_info.components[conan_component_alias].libdirs = []

        self.cpp_info.components["userver-core-internal"].defines.append(
            f'USERVER_NAMESPACE={self.options.namespace}',
        )
        self.cpp_info.components["userver-core-internal"].defines.append(
            f'USERVER_NAMESPACE_BEGIN={self.options.namespace_begin}',
        )
        self.cpp_info.components["userver-core-internal"].defines.append(
            f'USERVER_NAMESPACE_END={self.options.namespace_end}',
        )

        self.cpp_info.set_property("cmake_file_name", "userver") 

        add_components(self._userver_components)
        #self.cpp_info.components["core"].set_property("cmake_target_name", "userver-core")
        #self.cpp_info.components["core"].libs = ["userver-core", "userver-core-internal"]

        # self.cpp_info.components["universal"].set_property("cmake_target_name", "userver-universal")
        # self.cpp_info.components["universal"].libs = ["userver-universal"]

        # if self.options.with_grpc:
        #     self.cpp_info.components["grpc"].set_property("cmake_target_name", "userver-grpc")
        #     self.cpp_info.components["grpc"].libs = ["userver-grpc"]

        # if self.options.with_utest:
        #     self.cpp_info.components["utest"].set_property("cmake_target_name", "userver-utest")
        #     self.cpp_info.components["utest"].libs = ["userver-utest"]
        
        # if self.options.with_postgresql:
        #     self.cpp_info.components["postgresql"].set_property("cmake_target_name", "userver-postgresql")
        #     self.cpp_info.components["postgresql"].libs = ["userver-postgresql"]
            
        # if self.options.with_mongodb:
        #     self.cpp_info.components["mongo"].set_property("cmake_target_name", "userver-mongo")
        #     self.cpp_info.components["mongo"].libs = ["userver-mongo"]

        # if self.options.with_redis:
        #     self.cpp_info.components["redis"].set_property("cmake_target_name", "userver-redis")
        #     self.cpp_info.components["redis"].libs = ["userver-redis"]

        # if self.options.with_rabbitmq:
        #     self.cpp_info.components["rabbitmq"].set_property("cmake_target_name", "userver-rabbitmq")
        #     self.cpp_info.components["rabbitmq"].libs = ["userver-rabbitmq"]

        self.cpp_info.filenames["cmake_find_package"] = "userver"
        self.cpp_info.filenames["cmake_find_package_multi"] = "userver"