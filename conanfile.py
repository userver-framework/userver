# pylint: disable=no-member
import os

from conan import ConanFile
from conan.tools.cmake import CMake
from conan.tools.cmake import cmake_layout
from conan.tools.cmake import CMakeDeps
from conan.tools.cmake import CMakeToolchain
from conan.tools.files import copy
from conan.tools.files import replace_in_file

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
        'with_clickhouse': True,
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
        self.requires('cryptopp/8.7.0')
        self.requires('fmt/8.1.1')
        self.requires('libnghttp2/1.51.0')
        self.requires('libcurl/7.86.0')
        self.requires('libev/4.33')
        self.requires('http_parser/2.9.4')
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
        if self.options.with_clickhouse:
            self.requires('clickhouse-cpp/2.4.0')
            self.requires('abseil/20220623.0')
        if self.options.with_utest:
            self.requires('gtest/1.12.1')
            self.requires('benchmark/1.6.2')

    def generate(self):
        tool_ch = CMakeToolchain(self)
        tool_ch.variables['CMAKE_FIND_DEBUG_MODE'] = False

        tool_ch.variables['USERVER_OPEN_SOURCE_BUILD'] = True
        tool_ch.variables['USERVER_CONAN'] = True
        tool_ch.variables['USERVER_IS_THE_ROOT_PROJECT'] = False
        tool_ch.variables['USERVER_DOWNLOAD_PACKAGES'] = True
        tool_ch.variables['USERVER_FEATURE_DWCAS'] = True
        tool_ch.variables['USERVER_NAMESPACE'] = self.options.namespace
        tool_ch.variables[
            'USERVER_NAMESPACE_BEGIN'
        ] = self.options.namespace_begin
        tool_ch.variables['USERVER_NAMESPACE_END'] = self.options.namespace_end

        tool_ch.variables['USERVER_LTO'] = self.options.lto
        tool_ch.variables[
            'USERVER_FEATURE_JEMALLOC'
        ] = self.options.with_jemalloc
        tool_ch.variables[
            'USERVER_FEATURE_MONGODB'
        ] = self.options.with_mongodb
        tool_ch.variables[
            'USERVER_FEATURE_POSTGRESQL'
        ] = self.options.with_postgresql
        tool_ch.variables[
            'USERVER_FEATURE_PATCH_LIBPQ'
        ] = self.options.with_postgresql_extra
        tool_ch.variables['USERVER_FEATURE_REDIS'] = self.options.with_redis
        tool_ch.variables['USERVER_FEATURE_GRPC'] = self.options.with_grpc
        tool_ch.variables[
            'USERVER_FEATURE_CLICKHOUSE'
        ] = self.options.with_clickhouse
        tool_ch.variables[
            'USERVER_FEATURE_UNIVERSAL'
        ] = self.options.with_universal
        tool_ch.variables[
            'USERVER_FEATURE_RABBITMQ'
        ] = self.options.with_rabbitmq
        tool_ch.variables['USERVER_FEATURE_UTEST'] = self.options.with_utest
        tool_ch.variables[
            'USERVER_FEATURE_TESTSUITE'
        ] = self.options.with_utest

        tool_ch.generate()

        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    @property
    def _cmake_subfolder(self):
        return os.path.join(self.package_folder, 'cmake')

    def package(self):
        self.copy(pattern='LICENSE', dst='licenses')

        copy(
            self,
            pattern='*',
            dst=os.path.join(self.package_folder, 'include', 'shared'),
            src=os.path.join(self.source_folder, 'shared', 'include'),
            keep_path=True,
        )

        copy(
            self,
            pattern='*',
            dst=os.path.join(self.package_folder, 'scripts'),
            src=os.path.join(self.source_folder, 'scripts'),
            keep_path=True,
        )

        def copy_component(component):
            copy(
                self,
                pattern='*',
                dst=os.path.join(self.package_folder, 'include', component),
                src=os.path.join(self.source_folder, component, 'include'),
                keep_path=True,
            )
            copy(
                self,
                pattern='*.a',
                dst=os.path.join(self.package_folder, 'lib'),
                src=os.path.join(self._build_subfolder, component),
                keep_path=False,
            )
            copy(
                self,
                pattern='*.so',
                dst=os.path.join(self.package_folder, 'lib'),
                src=os.path.join(self._build_subfolder, component),
                keep_path=False,
            )

        copy_component('core')

        if self.options.with_universal:
            copy_component('universal')
        if self.options.with_grpc:
            copy_component('grpc')
            copy(
                self,
                pattern='*',
                dst=os.path.join(self.package_folder, 'include', 'grpc'),
                src=os.path.join(
                    self.source_folder, 'grpc', 'handlers', 'include',
                ),
                keep_path=True,
            )
            copy(
                self,
                pattern='GrpcTargets.cmake',
                dst=os.path.join(self.package_folder, 'cmake'),
                src=os.path.join(self.source_folder, 'cmake'),
                keep_path=True,
            )
            replace_in_file(
                self,
                os.path.join(
                    self.package_folder, 'cmake', 'GrpcTargets.cmake',
                ),
                'userver-grpc',
                'userver::grpc',
            )

            grpc_file = open(
                os.path.join(self.package_folder, 'cmake', 'GrpcConan.cmake'),
                'a+',
            )
            grpc_file.write('\nset(USERVER_CONAN TRUE)')
            grpc_file.write('\nset(PYTHON "python3")')
            grpc_file.close()
        if self.options.with_utest:
            copy(
                self,
                pattern='*',
                dst=os.path.join(self.package_folder, 'include', 'utest'),
                src=os.path.join(
                    self.source_folder, 'core', 'testing', 'include',
                ),
                keep_path=True,
            )
            copy(
                self,
                pattern='*',
                dst=os.path.join(self.package_folder, 'testsuite'),
                src=os.path.join(self.source_folder, 'testsuite'),
                keep_path=True,
            )
            copy(
                self,
                pattern='UserverTestsuite.cmake',
                dst=os.path.join(self.package_folder, 'cmake'),
                src=os.path.join(self.source_folder, 'cmake'),
                keep_path=True,
            )
            copy(
                self,
                pattern='AddGoogleTests.cmake',
                dst=os.path.join(self.package_folder, 'cmake'),
                src=os.path.join(self.source_folder, 'cmake'),
                keep_path=True,
            )
        if self.options.with_postgresql:
            copy_component('postgresql')

        if self.options.with_mongodb:
            copy_component('mongo')

        if self.options.with_redis:
            copy_component('redis')

        if self.options.with_rabbitmq:
            copy_component('rabbitmq')

        if self.options.with_clickhouse:
            copy_component('clickhouse')

    @property
    def _userver_components(self):
        def ares():
            return ['c-ares::c-ares']

        def fmt():
            return ['fmt::fmt']

        def curl():
            return ['libcurl::libcurl']

        def cryptopp():
            return ['cryptopp::cryptopp']

        def cctz():
            return ['cctz::cctz']

        def boost():
            return ['boost::boost']

        def concurrentqueue():
            return ['concurrentqueue::concurrentqueue']

        def yaml():
            return ['yaml-cpp::yaml-cpp']

        def libev():
            return ['libev::libev']

        def http_parser():
            return ['http_parser::http_parser']

        def openssl():
            return ['openssl::openssl']

        def jemalloc():
            return ['jemalloc::jemalloc'] if self.options.with_jemalloc else []

        def grpc():
            return ['grpc::grpc'] if self.options.with_grpc else []

        def postgresql():
            return ['libpq::pq'] if self.options.with_postgresql else []

        def gtest():
            return ['gtest::gtest'] if self.options.with_utest else []

        def benchmark():
            return ['benchmark::benchmark'] if self.options.with_utest else []

        def mongo():
            return (
                ['mongo-c-driver::mongo-c-driver']
                if self.options.with_mongodb
                else []
            )

        def hiredis():
            return ['hiredis::hiredis'] if self.options.with_redis else []

        def amqpcpp():
            return ['amqp-cpp::amqp-cpp'] if self.options.with_rabbitmq else []

        def clickhouse():
            return (
                ['clickhouse-cpp::clickhouse-cpp']
                if self.options.with_clickhouse
                else []
            )

        userver_components = [
            {
                'target': 'core',
                'lib': 'core',
                'requires': (
                    ['core-internal']
                    + fmt()
                    + cctz()
                    + boost()
                    + concurrentqueue()
                    + yaml()
                    + libev()
                    + http_parser()
                    + curl()
                    + cryptopp()
                    + jemalloc()
                    + ares()
                ),
            },
        ]
        if self.options.with_universal:
            userver_components.extend(
                [
                    {
                        'target': 'universal',
                        'lib': 'universal',
                        'requires': (
                            fmt()
                            + cctz()
                            + boost()
                            + concurrentqueue()
                            + yaml()
                            + cryptopp()
                            + jemalloc()
                            + openssl()
                        ),
                    },
                ],
            )
        if self.options.with_grpc:
            userver_components.extend(
                [
                    {
                        'target': 'grpc',
                        'lib': 'grpc',
                        'requires': ['core'] + grpc(),
                    },
                    {
                        'target': 'grpc-handlers',
                        'lib': 'grpc-handlers',
                        'requires': ['core'] + grpc(),
                    },
                    {
                        'target': 'grpc-handlers_proto',
                        'lib': 'grpc-handlers_proto',
                        'requires': ['core'] + grpc(),
                    },
                    {
                        'target': 'api-common-protos',
                        'lib': 'api-common-protos',
                        'requires': ['grpc'],
                    },
                ],
            )
        if self.options.with_utest:
            userver_components.extend(
                [
                    {
                        'target': 'utest',
                        'lib': 'utest',
                        'requires': ['core'] + gtest(),
                    },
                    {
                        'target': 'ubench',
                        'lib': 'ubench',
                        'requires': ['core'] + benchmark(),
                    },
                ],
            )
        if self.options.with_postgresql:
            userver_components.extend(
                [
                    {
                        'target': 'postgresql',
                        'lib': 'postgresql',
                        'requires': ['core'] + postgresql(),
                    },
                ],
            )
        if self.options.with_mongodb:
            userver_components.extend(
                [
                    {
                        'target': 'mongo',
                        'lib': 'mongo',
                        'requires': ['core'] + mongo(),
                    },
                ],
            )
        if self.options.with_redis:
            userver_components.extend(
                [
                    {
                        'target': 'redis',
                        'lib': 'redis',
                        'requires': ['core'] + hiredis(),
                    },
                ],
            )
        if self.options.with_rabbitmq:
            userver_components.extend(
                [
                    {
                        'target': 'rabbitmq',
                        'lib': 'rabbitmq',
                        'requires': ['core'] + amqpcpp(),
                    },
                ],
            )
        if self.options.with_clickhouse:
            userver_components.extend(
                [
                    {
                        'target': 'clickhouse',
                        'lib': 'clickhouse',
                        'requires': ['core'] + clickhouse(),
                    },
                ],
            )
        return userver_components

    def package_info(self):

        debug = (
            'd'
            if self.settings.build_type == 'Debug'
            and self.settings.os == 'Windows'
            else ''
        )

        def get_lib_name(module):
            return f'userver-{module}{debug}'

        def add_components(components):
            for component in components:
                conan_component = component['target']
                cmake_target = component['target']
                cmake_component = component['lib']
                lib_name = get_lib_name(component['lib'])
                requires = component['requires']
                # TODO: we should also define COMPONENTS names of each target
                # for find_package() but not possible yet in CMakeDeps
                #       see https://github.com/conan-io/conan/issues/10258
                self.cpp_info.components[conan_component].set_property(
                    'cmake_target_name', 'userver::' + cmake_target,
                )
                if cmake_component == 'grpc':
                    self.cpp_info.components[conan_component].libs.append(
                        get_lib_name('grpc-internal'),
                    )
                else:
                    self.cpp_info.components[conan_component].libs = [lib_name]
                if cmake_component == 'core':
                    self.cpp_info.components[conan_component].libs.append(
                        get_lib_name('core-internal'),
                    )
                if cmake_component != 'ubench':
                    self.cpp_info.components[
                        conan_component
                    ].includedirs.append(
                        os.path.join('include', cmake_component),
                    )
                if cmake_component == 'core' or cmake_component == 'universal':
                    self.cpp_info.components[
                        conan_component
                    ].includedirs.append(os.path.join('include', 'shared'))

                self.cpp_info.components[conan_component].requires = requires

        self.cpp_info.components['core-internal'].defines.append(
            f'USERVER_NAMESPACE={self.options.namespace}',
        )
        self.cpp_info.components['core-internal'].defines.append(
            f'USERVER_NAMESPACE_BEGIN={self.options.namespace_begin}',
        )
        self.cpp_info.components['core-internal'].defines.append(
            f'USERVER_NAMESPACE_END={self.options.namespace_end}',
        )

        self.cpp_info.components['universal'].defines.append(
            f'USERVER_NAMESPACE={self.options.namespace}',
        )
        self.cpp_info.components['universal'].defines.append(
            f'USERVER_NAMESPACE_BEGIN={self.options.namespace_begin}',
        )
        self.cpp_info.components['universal'].defines.append(
            f'USERVER_NAMESPACE_END={self.options.namespace_end}',
        )

        self.cpp_info.set_property('cmake_file_name', 'userver')

        add_components(self._userver_components)

        build_modules = [
            os.path.join(self._cmake_subfolder, 'UserverTestsuite.cmake'),
        ]
        if self.options.with_utest:
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'AddGoogleTests.cmake'),
            )
        if self.options.with_grpc:
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'GrpcConan.cmake'),
            )
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'GrpcTargets.cmake'),
            )

        self.cpp_info.set_property('cmake_build_modules', build_modules)
