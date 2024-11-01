# pylint: disable=no-member
import os
import re

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake
from conan.tools.cmake import cmake_layout
from conan.tools.cmake import CMakeDeps
from conan.tools.cmake import CMakeToolchain
from conan.tools.files import copy
from conan.tools.files import load

required_conan_version = '>=2.8.0'  # pylint: disable=invalid-name


class UserverConan(ConanFile):
    name = 'userver'
    description = 'The C++ Asynchronous Framework'
    topics = ('framework', 'coroutines', 'asynchronous')
    url = 'https://github.com/userver-framework/userver'
    homepage = 'https://userver.tech/'
    license = 'Apache-2.0'
    package_type = 'static-library'
    exports_sources = '*'

    settings = 'os', 'arch', 'compiler', 'build_type'
    options = {
        'fPIC': [True, False],
        'lto': [True, False],
        'with_jemalloc': [True, False],
        'with_mongodb': [True, False],
        'with_postgresql': [True, False],
        'with_postgresql_extra': [True, False],
        'with_redis': [True, False],
        'with_grpc': [True, False],
        'with_clickhouse': [True, False],
        'with_rabbitmq': [True, False],
        'with_utest': [True, False],
        'with_kafka': [True, False],
        'with_otlp': [True, False],
        'namespace': ['ANY'],
        'namespace_begin': ['ANY'],
        'namespace_end': ['ANY'],
    }

    default_options = {
        'fPIC': True,
        'lto': False,
        'with_jemalloc': True,
        'with_mongodb': True,
        'with_postgresql': True,
        'with_postgresql_extra': False,
        'with_redis': True,
        'with_grpc': True,
        'with_clickhouse': True,
        'with_rabbitmq': True,
        'with_utest': True,
        'with_kafka': True,
        'with_otlp': True,
        'namespace': 'userver',
        'namespace_begin': 'namespace userver {',
        'namespace_end': '}',
        'mongo-c-driver/*:with_sasl': 'cyrus',
        'grpc/*:php_plugin': False,
        'grpc/*:node_plugin': False,
        'grpc/*:ruby_plugin': False,
        'grpc/*:csharp_plugin': False,
        'grpc/*:objective_c_plugin': False,
    }

    # scm = {
    #     'type': 'git',
    #     'url': 'https://github.com/userver-framework/userver.git',
    #     'revision': 'develop'
    # }

    def set_version(self):
        content = load(
            self,
            os.path.join(
                os.path.dirname(os.path.realpath(__file__)),
                'cmake/GetUserverVersion.cmake',
            ),
        )
        major_version = (
            re.search(r'set\(USERVER_MAJOR_VERSION (.*)\)', content)
            .group(1)
            .strip()
        )
        minor_version = (
            re.search(r'set\(USERVER_MINOR_VERSION (.*)\)', content)
            .group(1)
            .strip()
        )

        self.version = f'{major_version}.{minor_version}'

    @property
    def _source_subfolder(self):
        return 'source'

    @property
    def _build_subfolder(self):
        return os.path.join(self.build_folder)

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires('boost/1.86.0', transitive_headers=True)
        self.requires('c-ares/1.34.1')
        self.requires('cctz/2.4', transitive_headers=True)
        self.requires('concurrentqueue/1.0.3', transitive_headers=True)
        self.requires('cryptopp/8.9.0')
        self.requires('fmt/8.1.1', transitive_headers=True)
        self.requires('libnghttp2/1.61.0')
        self.requires('libcurl/8.10.1')
        self.requires('libev/4.33')
        self.requires('openssl/3.2.2')
        self.requires('rapidjson/cci.20220822', transitive_headers=True)
        self.requires('yaml-cpp/0.8.0')
        self.requires('zlib/1.3.1')
        self.requires('zstd/1.5.5')

        if self.options.with_jemalloc:
            self.requires('jemalloc/5.3.0')
        if self.options.with_grpc or self.options.with_clickhouse:
            self.requires(
                'abseil/20230802.1',
                force=True,
                transitive_headers=True,
                transitive_libs=True,
            )
        if self.options.with_grpc:
            self.requires(
                'grpc/1.54.3', transitive_headers=True, transitive_libs=True,
            )
            self.requires(
                'grpc-proto/cci.20220627', transitive_headers=True, transitive_libs=True,
            )
            self.requires(
                'googleapis/cci.20230501', transitive_headers=True, transitive_libs=True,
            )
            self.requires(
                'protobuf/3.21.12', transitive_headers=True, transitive_libs=True,
            )
        if self.options.with_postgresql:
            self.requires('libpq/14.5')
        if self.options.with_mongodb or self.options.with_kafka:
            self.requires('cyrus-sasl/2.1.28', force=True)
        if self.options.with_mongodb:
            self.requires(
                'mongo-c-driver/1.28.0',
                transitive_headers=True,
                transitive_libs=True,
            )
        if self.options.with_redis:
            self.requires('hiredis/1.2.0')
        if self.options.with_rabbitmq:
            self.requires('amqp-cpp/4.3.26')
        if self.options.with_clickhouse:
            self.requires('clickhouse-cpp/2.5.1')
        if self.options.with_utest:
            self.requires(
                'gtest/1.15.0', transitive_headers=True, transitive_libs=True,
            )
            self.requires(
                'benchmark/1.9.0',
                transitive_headers=True,
                transitive_libs=True,
            )
        if self.options.with_kafka:
            self.requires('librdkafka/2.6.0')

    def validate(self):
        if self.settings.os == 'Windows':
            raise ConanInvalidConfiguration(
                'userver cannot be built on Windows',
            )

        if (
            self.options.with_mongodb
            and self.dependencies['mongo-c-driver'].options.with_sasl
            != 'cyrus'
        ):
            raise ConanInvalidConfiguration(
                f'{self.ref} requires mongo-c-driver with_sasl cyrus',
            )

    def generate(self):
        tool_ch = CMakeToolchain(self)
        tool_ch.variables['CMAKE_FIND_DEBUG_MODE'] = False

        tool_ch.variables['USERVER_CONAN'] = True
        tool_ch.variables['USERVER_IS_THE_ROOT_PROJECT'] = False
        tool_ch.variables['USERVER_DOWNLOAD_PACKAGES'] = True
        tool_ch.variables['USERVER_FEATURE_DWCAS'] = True
        tool_ch.variables['USERVER_NAMESPACE'] = self.options.namespace
        tool_ch.variables['USERVER_NAMESPACE_BEGIN'] = (
            self.options.namespace_begin
        )
        tool_ch.variables['USERVER_NAMESPACE_END'] = self.options.namespace_end

        tool_ch.variables['USERVER_LTO'] = self.options.lto
        tool_ch.variables['USERVER_FEATURE_JEMALLOC'] = (
            self.options.with_jemalloc
        )
        tool_ch.variables['USERVER_FEATURE_MONGODB'] = (
            self.options.with_mongodb
        )
        tool_ch.variables['USERVER_FEATURE_POSTGRESQL'] = (
            self.options.with_postgresql
        )
        tool_ch.variables['USERVER_FEATURE_PATCH_LIBPQ'] = (
            self.options.with_postgresql_extra
        )
        tool_ch.variables['USERVER_FEATURE_REDIS'] = self.options.with_redis
        tool_ch.variables['USERVER_FEATURE_GRPC'] = self.options.with_grpc
        tool_ch.variables['USERVER_FEATURE_CLICKHOUSE'] = (
            self.options.with_clickhouse
        )
        tool_ch.variables['USERVER_FEATURE_RABBITMQ'] = (
            self.options.with_rabbitmq
        )
        tool_ch.variables['USERVER_FEATURE_UTEST'] = self.options.with_utest
        tool_ch.variables['USERVER_FEATURE_TESTSUITE'] = (
            self.options.with_utest
        )
        tool_ch.variables['USERVER_FEATURE_KAFKA'] = self.options.with_kafka
        tool_ch.variables['USERVER_FEATURE_OTLP'] = self.options.with_otlp
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
        copy(self, pattern='LICENSE', src=self.source_folder, dst='licenses')

        copy(
            self,
            pattern='*',
            dst=os.path.join(self.package_folder, 'scripts'),
            src=os.path.join(self.source_folder, 'scripts'),
            keep_path=True,
        )

        copy(
            self,
            pattern='*',
            dst=os.path.join(
                self.package_folder, 'include', 'function_backports',
            ),
            src=os.path.join(
                self.source_folder,
                'third_party',
                'function_backports',
                'include',
            ),
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
        copy_component('universal')
        for cmake_file in (
            'UserverSetupEnvironment',
            'SetupLinker',
            'SetupLTO',
            'UserverVenv',
        ):
            copy(
                self,
                pattern=f'{cmake_file}.cmake',
                dst=os.path.join(self.package_folder, 'cmake'),
                src=os.path.join(self.source_folder, 'cmake'),
                keep_path=True,
            )

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

            with open(
                os.path.join(self.package_folder, 'cmake', 'GrpcConan.cmake'),
                'a+',
            ) as grpc_file:
                grpc_file.write('\nset(USERVER_CONAN TRUE)')
        if self.options.with_utest:
            copy(
                self,
                pattern='*',
                dst=os.path.join(self.package_folder, 'include', 'utest'),
                src=os.path.join(
                    self.source_folder, 'universal', 'utest', 'include',
                ),
                keep_path=True,
            )
            copy(
                self,
                pattern='*',
                dst=os.path.join(self.package_folder, 'include', 'utest'),
                src=os.path.join(
                    self.source_folder, 'core', 'utest', 'include',
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
                pattern='AddGoogleTests.cmake',
                dst=os.path.join(self.package_folder, 'cmake'),
                src=os.path.join(self.source_folder, 'cmake'),
                keep_path=True,
            )
        if self.options.with_grpc or self.options.with_utest:
            copy(
                self,
                pattern='UserverTestsuite.cmake',
                dst=os.path.join(self.package_folder, 'cmake'),
                src=os.path.join(self.source_folder, 'cmake'),
                keep_path=True,
            )
            copy(
                self,
                pattern='SetupProtobuf.cmake',
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

        if self.options.with_kafka:
            copy_component('kafka')

        if self.options.with_otlp:
            copy_component('otlp')

    @property
    def _userver_components(self):
        def abseil():
            return ['abseil::abseil']

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

        def libnghttp2():
            return ['libnghttp2::libnghttp2']

        def openssl():
            return ['openssl::openssl']

        def rapidjson():
            return ['rapidjson::rapidjson']

        def zlib():
            return ['zlib::zlib']

        def zstd():
            # According to https://conan.io/center/recipes/zstd should be
            # zstd::libzstd_static, but it does not work that way
            return ['zstd::zstd']

        def jemalloc():
            return ['jemalloc::jemalloc'] if self.options.with_jemalloc else []

        def grpc():
            return ['grpc::grpc'] if self.options.with_grpc else []

        def googleapis():
            return ['googleapis::googleapis'] if self.options.with_grpc else []

        def grpcproto():
            return ['grpc-proto::grpc-proto'] if self.options.with_grpc else []

        def protobuf():
            return ['protobuf::protobuf'] if self.options.with_grpc else []

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

        def cyrussasl():
            return (
                ['cyrus-sasl::cyrus-sasl'] if self.options.with_mongodb else []
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

        def librdkafka():
            return (
                ['librdkafka::librdkafka'] if self.options.with_kafka else []
            )

        userver_components = [
            {
                'target': 'core',
                'lib': 'core',
                'requires': (
                    ['universal']
                    + abseil()
                    + fmt()
                    + cctz()
                    + boost()
                    + concurrentqueue()
                    + yaml()
                    + libev()
                    + libnghttp2()
                    + curl()
                    + cryptopp()
                    + jemalloc()
                    + ares()
                    + rapidjson()
                    + zlib()
                ),
            },
        ]
        userver_components.extend([
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
                    + zstd()
                ),
            },
        ])

        if self.options.with_grpc:
            userver_components.extend([
                {
                    'target': 'grpc',
                    'lib': 'grpc',
                    'requires': (
                        ['core']
                        + grpc()
                        + protobuf()
                        + googleapis()
                        + grpcproto()
                    ),
                },
                {
                    'target': 'grpc-handlers',
                    'lib': 'grpc-handlers',
                    'requires': ['core'] + grpc(),
                },
                {
                    'target': 'grpc-handlers-proto',
                    'lib': 'grpc-handlers-proto',
                    'requires': ['core'] + grpc(),
                },
                {
                    'target': 'api-common-protos',
                    'lib': 'api-common-protos',
                    'requires': ['grpc'],
                },
            ])
        if self.options.with_utest:
            userver_components.extend([
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
            ])
        if self.options.with_postgresql:
            userver_components.extend([
                {
                    'target': 'postgresql',
                    'lib': 'postgresql',
                    'requires': ['core'] + postgresql(),
                },
            ])
        if self.options.with_mongodb:
            userver_components.extend([
                {
                    'target': 'mongo',
                    'lib': 'mongo',
                    'requires': ['core'] + mongo() + cyrussasl(),
                },
            ])
        if self.options.with_redis:
            userver_components.extend([
                {
                    'target': 'redis',
                    'lib': 'redis',
                    'requires': ['core'] + hiredis(),
                },
            ])
        if self.options.with_rabbitmq:
            userver_components.extend([
                {
                    'target': 'rabbitmq',
                    'lib': 'rabbitmq',
                    'requires': ['core'] + amqpcpp(),
                },
            ])
        if self.options.with_clickhouse:
            userver_components.extend([
                {
                    'target': 'clickhouse',
                    'lib': 'clickhouse',
                    'requires': ['core'] + clickhouse(),
                },
            ])
        if self.options.with_kafka:
            userver_components.extend([
                {
                    'target': 'kafka',
                    'lib': 'kafka',
                    'requires': (
                        ['core']
                        + cyrussasl()
                        + curl()
                        + zlib()
                        + openssl()
                        + librdkafka()
                    ),
                },
            ])

        if self.options.with_otlp:
            userver_components.extend([
                {'target': 'otlp', 'lib': 'otlp', 'requires': ['core']},
            ])
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
                    self.cpp_info.components[conan_component].libs.append(
                        get_lib_name('grpc-proto'),
                    )
                else:
                    self.cpp_info.components[conan_component].libs = [lib_name]
                if cmake_component == 'otlp':
                    self.cpp_info.components[conan_component].libs.append(
                        get_lib_name('otlp-proto'),
                    )
                if cmake_component == 'universal':
                    self.cpp_info.components[
                        cmake_component
                    ].includedirs.append(
                        os.path.join('include', 'function_backports'),
                    )
                if cmake_component != 'ubench':
                    self.cpp_info.components[
                        conan_component
                    ].includedirs.append(
                        os.path.join('include', cmake_component),
                    )

                self.cpp_info.components[conan_component].requires = requires

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

        with open(
            os.path.join(self._cmake_subfolder, 'CallSetupEnv.cmake'), 'w',
        ) as cmake_file:
            cmake_file.write('userver_setup_environment()')

        build_modules = [
            os.path.join(
                self._cmake_subfolder, 'UserverSetupEnvironment.cmake',
            ),
            os.path.join(self._cmake_subfolder, 'CallSetupEnv.cmake'),
            os.path.join(self._cmake_subfolder, 'UserverVenv.cmake'),
            os.path.join(self._cmake_subfolder, 'UserverTestsuite.cmake'),
        ]
        if self.options.with_utest:
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'AddGoogleTests.cmake'),
            )
        if self.options.with_grpc:
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'SetupProtobuf.cmake'),
            )
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'GrpcConan.cmake'),
            )
            build_modules.append(
                os.path.join(self._cmake_subfolder, 'GrpcTargets.cmake'),
            )

        self.cpp_info.set_property('cmake_build_modules', build_modules)
