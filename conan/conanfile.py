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
    generators = "cmake_find_package"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False], "fPIC": [True, False], 
        "with_jemalloc": [True, False],
        "with_mongodb": [True, False],
        "with_postgresql": [True, False],
        "with_redis": [True, False],
        "with_grpc": [True, False],
        "with_clickhouse": [True, False],
        "with_universal": [True, False]
    }

    default_options = {
        "shared": False, "fPIC": True, 
        "with_jemalloc": True,
        "with_mongodb": False, 
        "with_postgresql": False,
        "with_redis": False,
        "with_grpc": False, 
        "with_clickhouse": False,
        "with_universal": False,
    }

    scm = {
        "type": "git",
        "url": "https://github.com/jorgenpo/userver.git",
        "revision": "conan"
    }

    def configure(self):
        if self.options.shared:
            del self.options.fPIC

    def requirements(self):
        self.requires("boost/1.79.0")
        self.requires("libev/4.33")
        self.requires("spdlog/1.9.0")
        self.requires("fmt/8.1.1")
        self.requires("c-ares/1.18.1")
        self.requires("libcurl/7.68.0")
        self.requires("cryptopp/8.6.0")
        self.requires("yaml-cpp/0.7.0")
        self.requires("cctz/2.3")
        self.requires("http_parser/2.9.4")

        if self.options.with_jemalloc:
            self.requires("jemalloc/5.2.1")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_FIND_DEBUG_MODE"] = "OFF"

        cmake.definitions["USERVER_OPEN_SOURCE_BUILD"] = "ON"
        cmake.definitions["USERVER_IS_THE_ROOT_PROJECT"] = "OFF"
        cmake.definitions["USERVER_DOWNLOAD_PACKAGES"] = "OFF"

        if not self.options.with_jemalloc:
            cmake.definitions["USERVER_FEATURE_JEMALLOC"] = "OFF"
        if not self.options.with_mongodb:
            cmake.definitions["USERVER_FEATURE_MONGODB"] = "OFF"
        if not self.options.with_postgresql:
            cmake.definitions["USERVER_FEATURE_POSTGRESQL"] = "OFF"
        if not self.options.with_redis:
            cmake.definitions["USERVER_FEATURE_REDIS"] = "OFF"
        if not self.options.with_grpc:
            cmake.definitions["USERVER_FEATURE_GRPC"] = "OFF"
        if not self.options.with_clickhouse:
            cmake.definitions["USERVER_FEATURE_CLICKHOUSE"] = "OFF"
        if not self.options.with_universal:
            cmake.definitions["USERVER_FEATURE_UNIVERSAL"] = "OFF"

        cmake.configure(build_folder="cmake-build")
        return cmake

    def build(self):
        # Rename some packages for cmake to find them in find_package
        os.rename("Findlibev.cmake", "FindLibEv.cmake")
        os.rename("Findcryptopp.cmake", "FindCryptoPP.cmake")
        os.rename("Findyaml-cpp.cmake", "Findlibyamlcpp.cmake")
        os.rename("Findhttp_parser.cmake", "FindHttp_Parser.cmake")
        
        if self.options.with_jemalloc:
            os.rename("Findjemalloc.cmake", "FindJemalloc.cmake")
            tools.replace_in_file("FindJemalloc.cmake", "jemalloc::jemalloc", "Jemalloc")

        # Tune Find* files to produce correct variables-
        tools.replace_in_file("FindCryptoPP.cmake", "cryptopp_FOUND", "CryptoPP_FOUND")
        tools.replace_in_file("FindCryptoPP.cmake", "cryptopp_VERSION", "CryptoPP_VERSION")
        tools.replace_in_file("FindCryptoPP.cmake", "cryptopp::cryptopp", "CryptoPP")
        tools.replace_in_file("Findlibyamlcpp.cmake", "yaml-cpp_FOUND", "libyamlcpp_FOUND")
        tools.replace_in_file("Findlibyamlcpp.cmake", "yaml-cpp_VERSION", "libyamlcpp_VERSION")
        tools.replace_in_file("Findlibyamlcpp.cmake", "yaml-cpp::yaml-cpp", "libyamlcpp")
        tools.replace_in_file("Findcctz.cmake", "cctz::cctz", "cctz")
        tools.replace_in_file("Findc-ares.cmake", "c-ares::c-ares", "c-ares")
        tools.replace_in_file("FindLibEv.cmake", "libev::libev", "LibEv")
        tools.replace_in_file("FindHttp_Parser.cmake", "http_parser::http_parser", "Http_Parser")

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
