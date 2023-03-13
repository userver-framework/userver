# pylint: disable=no-member

import os

import conans  # pylint: disable=import-error

required_conan_version = '>=1.51.0, <2.0.0'  # pylint: disable=invalid-name


class UserverConan(conans.ConanFile):
    name = 'userver'
    version = '1.0.0'
    description = 'The C++ Asynchronous Framework'
    topics = ('framework', 'coroutines', 'asynchronous')
    url = 'https://github.com/userver-framework/userver'
    homepage = 'https://userver.tech/'
    license = 'Apache-2.0'
    exports_sources = '*'
    generators = ('cmake_find_package', 'cmake_paths')

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
        return 'build_subfolder'

    def configure(self):
        if self.options.shared:
            del self.options.fPIC

        if not self.options.namespace_begin:
            self.options.namespace_begin = (
                f'namespace {self.options.namespace} {{'
            )
        if not self.options.namespace_end:
            self.options.namespace_end = '}'

    def requirements(self):
        self.requires('boost/1.79.0')
        self.requires('libev/4.33')
        self.requires('spdlog/1.9.0')
        self.options['spdlog'].header_only = True
        self.requires('fmt/8.1.1')
        self.requires('c-ares/1.18.1')
        self.requires('libcurl/7.86.0')
        self.requires('cryptopp/8.6.0')
        self.requires('yaml-cpp/0.7.0')
        self.requires('cctz/2.3')
        self.requires('http_parser/2.9.4')
        self.requires('openssl/1.1.1s')
        self.requires('rapidjson/cci.20220822')
        self.requires('concurrentqueue/1.0.3')
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

    def _configure_cmake(self):
        cmake = conans.CMake(self)
        cmake.definitions['CMAKE_FIND_DEBUG_MODE'] = 'OFF'

        cmake.definitions['USERVER_IS_THE_ROOT_PROJECT'] = 'OFF'
        cmake.definitions['USERVER_DOWNLOAD_PACKAGES'] = 'ON'
        cmake.definitions['USERVER_FEATURE_DWCAS'] = 'ON'
        cmake.definitions['USERVER_NAMESPACE'] = self.options.namespace
        cmake.definitions[
            'USERVER_NAMESPACE_BEGIN'
        ] = self.options.namespace_begin
        cmake.definitions['USERVER_NAMESPACE_END'] = self.options.namespace_end

        if not self.options.lto:
            cmake.definitions['USERVER_LTO'] = 'OFF'
        if not self.options.with_jemalloc:
            cmake.definitions['USERVER_FEATURE_JEMALLOC'] = 'OFF'
        if not self.options.with_mongodb:
            cmake.definitions['USERVER_FEATURE_MONGODB'] = 'OFF'
        if not self.options.with_postgresql:
            cmake.definitions['USERVER_FEATURE_POSTGRESQL'] = 'OFF'
        if not self.options.with_postgresql_extra:
            cmake.definitions['USERVER_FEATURE_PATCH_LIBPQ'] = 'OFF'
        if not self.options.with_redis:
            cmake.definitions['USERVER_FEATURE_REDIS'] = 'OFF'
        if not self.options.with_grpc:
            cmake.definitions['USERVER_FEATURE_GRPC'] = 'OFF'
        if not self.options.with_clickhouse:
            cmake.definitions['USERVER_FEATURE_CLICKHOUSE'] = 'OFF'
        if not self.options.with_universal:
            cmake.definitions['USERVER_FEATURE_UNIVERSAL'] = 'OFF'
        if not self.options.with_rabbitmq:
            cmake.definitions['USERVER_FEATURE_RABBITMQ'] = 'OFF'
        if not self.options.with_utest:
            cmake.definitions['USERVER_FEATURE_UTEST'] = 'OFF'

        cmake.configure(build_folder=self._build_subfolder)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern='LICENSE', dst='licenses')

        self.copy(
            pattern='*', dst='include', src='core/include', keep_path=True,
        )
        self.copy(
            pattern='*', dst='include', src='shared/include', keep_path=True,
        )

        if self.options.with_grpc:
            self.copy(
                pattern='*', dst='include', src='grpc/include', keep_path=True,
            )
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
        if self.options.with_postgresql:
            self.copy(
                pattern='*',
                dst='include',
                src='postgresql/include',
                keep_path=True,
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
        if self.options.with_mongodb:
            self.copy(
                pattern='*',
                dst='include',
                src='mongo/include',
                keep_path=True,
            )
        if self.options.with_redis:
            self.copy(
                pattern='*',
                dst='include',
                src='redis/include',
                keep_path=True,
            )
        if self.options.with_rabbitmq:
            self.copy(
                pattern='*',
                dst='include',
                src='rabbitmq/include',
                keep_path=True,
            )
        if self.options.with_clickhouse:
            self.copy(
                pattern='*',
                dst='include',
                src='clickhouse/include',
                keep_path=True,
            )

        self.copy(
            pattern='*.a',
            dst='lib',
            src=os.path.join(self._build_subfolder, 'userver'),
            keep_path=False,
        )
        self.copy(
            pattern='*.so',
            dst='lib',
            src=os.path.join(self._build_subfolder, 'userver'),
            keep_path=False,
        )

    def package_info(self):
        self.cpp_info.libs = conans.tools.collect_libs(self)

        self.cpp_info.defines.append(
            f'USERVER_NAMESPACE={self.options.namespace}',
        )
        self.cpp_info.defines.append(
            f'USERVER_NAMESPACE_BEGIN={self.options.namespace_begin}',
        )
        self.cpp_info.defines.append(
            f'USERVER_NAMESPACE_END={self.options.namespace_end}',
        )
