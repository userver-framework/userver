

# Conan automatically generated toolchain file
# DO NOT EDIT MANUALLY, it will be overwritten

# Avoid including toolchain file several times (bad if appending to variables like
#   CMAKE_CXX_FLAGS. See https://github.com/android/ndk/issues/323
include_guard()

message(STATUS "Using Conan toolchain: ${CMAKE_CURRENT_LIST_FILE}")

if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeToolchain' generator only works with CMake >= 3.15")
endif()










# Set the architectures for which to build.
set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "" FORCE)
# Setting CMAKE_OSX_SYSROOT SDK, when using Xcode generator the name is enough
# but full path is necessary for others
set(CMAKE_OSX_SYSROOT macosx CACHE STRING "" FORCE)
set(BITCODE "")
set(FOBJC_ARC "")
set(VISIBILITY "")
#Check if Xcode generator is used, since that will handle these flags automagically
if(CMAKE_GENERATOR MATCHES "Xcode")
  message(DEBUG "Not setting any manual command-line buildflags, since Xcode is selected as generator.")
else()
    string(APPEND CONAN_C_FLAGS " ${BITCODE} ${FOBJC_ARC}")
    string(APPEND CONAN_CXX_FLAGS " ${BITCODE} ${VISIBILITY} ${FOBJC_ARC}")
endif()

string(APPEND CONAN_CXX_FLAGS " -m64")
string(APPEND CONAN_C_FLAGS " -m64")
string(APPEND CONAN_SHARED_LINKER_FLAGS " -m64")
string(APPEND CONAN_EXE_LINKER_FLAGS " -m64")

string(APPEND CONAN_CXX_FLAGS " -stdlib=libc++")


# Extra c, cxx, linkflags and defines


if(DEFINED CONAN_CXX_FLAGS)
  string(APPEND CMAKE_CXX_FLAGS_INIT " ${CONAN_CXX_FLAGS}")
endif()
if(DEFINED CONAN_C_FLAGS)
  string(APPEND CMAKE_C_FLAGS_INIT " ${CONAN_C_FLAGS}")
endif()
if(DEFINED CONAN_SHARED_LINKER_FLAGS)
  string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " ${CONAN_SHARED_LINKER_FLAGS}")
endif()
if(DEFINED CONAN_EXE_LINKER_FLAGS)
  string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " ${CONAN_EXE_LINKER_FLAGS}")
endif()

get_property( _CMAKE_IN_TRY_COMPILE GLOBAL PROPERTY IN_TRY_COMPILE )
if(_CMAKE_IN_TRY_COMPILE)
    message(STATUS "Running toolchain IN_TRY_COMPILE")
    return()
endif()

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Definition of CMAKE_MODULE_PATH
# Explicitly defined "builddirs" of "host" dependencies
list(PREPEND CMAKE_MODULE_PATH "/Users/alexiprof/.conan/data/protobuf/3.21.4/_/_/package/8264a6a82f1a58ddbf1d543f69f549b8e37e8e6a/lib/cmake/protobuf")
# The root (which is the default builddirs) path of dependencies in the host context
list(PREPEND CMAKE_MODULE_PATH "/Users/alexiprof/.conan/data/cctz/2.3/_/_/package/6f1931e5860f734d55456a9298992b6b0b094a59/" "/Users/alexiprof/.conan/data/concurrentqueue/1.0.3/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/" "/Users/alexiprof/.conan/data/libev/4.33/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/" "/Users/alexiprof/.conan/data/http_parser/2.9.4/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/" "/Users/alexiprof/.conan/data/rapidjson/cci.20220822/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/" "/Users/alexiprof/.conan/data/spdlog/1.9.0/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/" "/Users/alexiprof/.conan/data/yaml-cpp/0.7.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/" "/Users/alexiprof/.conan/data/zlib/1.2.13/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/" "/Users/alexiprof/.conan/data/jemalloc/5.2.1/_/_/package/63ef65eed81da45031b3c1f57398571a636e3b31/" "/Users/alexiprof/.conan/data/amqp-cpp/4.3.16/_/_/package/9955bffe79339c531342a3a20c0a10a1e5a3a2e0/" "/Users/alexiprof/.conan/data/bzip2/1.0.8/_/_/package/4d994402bb6d6e8babdc89ce439cd1a0ab9877cd/" "/Users/alexiprof/.conan/data/libbacktrace/cci.20210118/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/" "/Users/alexiprof/.conan/data/libiconv/1.17/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/" "/Users/alexiprof/.conan/data/re2/20220601/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/" "/Users/alexiprof/.conan/data/grpc-proto/cci.20220627/_/_/package/4b7ed5be66f388b91704f85ba054bc3a1947e5f0/" "/Users/alexiprof/.conan/data/cyrus-sasl/2.1.27/_/_/package/7e7af0ed7931a161b325bff1e2f6c62e8ff7cc0a/")
# the generators folder (where conan generates files, like this toolchain)
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Definition of CMAKE_PREFIX_PATH, CMAKE_XXXXX_PATH
# The explicitly defined "builddirs" of "host" context dependencies must be in PREFIX_PATH
list(PREPEND CMAKE_PREFIX_PATH "/Users/alexiprof/.conan/data/protobuf/3.21.4/_/_/package/8264a6a82f1a58ddbf1d543f69f549b8e37e8e6a/lib/cmake/protobuf")
# The Conan local "generators" folder, where this toolchain is saved.
list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_LIST_DIR} )
list(PREPEND CMAKE_LIBRARY_PATH "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/lib" "/Users/alexiprof/.conan/data/boost/1.79.0/_/_/package/effc61087c57d5292f5fa96377e7b5e2c60734c9/lib" "/Users/alexiprof/.conan/data/c-ares/1.18.1/_/_/package/1b6931ff6fdc18b784cabfbe6f3b36facd6c1cd9/lib" "/Users/alexiprof/.conan/data/cctz/2.3/_/_/package/6f1931e5860f734d55456a9298992b6b0b094a59/lib" "/Users/alexiprof/.conan/data/cryptopp/8.7.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/lib" "/Users/alexiprof/.conan/data/fmt/8.1.1/_/_/package/80138d4a58def120da0b8c9199f2b7a4e464a85b/lib" "/Users/alexiprof/.conan/data/libnghttp2/1.51.0/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/libcurl/7.86.0/_/_/package/a097455223234e250d01a2687cf7c15446fbd5d5/lib" "/Users/alexiprof/.conan/data/libev/4.33/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/http_parser/2.9.4/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/openssl/1.1.1s/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/rapidjson/cci.20220822/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/lib" "/Users/alexiprof/.conan/data/spdlog/1.9.0/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/lib" "/Users/alexiprof/.conan/data/yaml-cpp/0.7.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/lib" "/Users/alexiprof/.conan/data/zlib/1.2.13/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/jemalloc/5.2.1/_/_/package/63ef65eed81da45031b3c1f57398571a636e3b31/lib" "/Users/alexiprof/.conan/data/grpc/1.48.0/_/_/package/747f375e854e326b233e80494b3c63476474d77a/lib" "/Users/alexiprof/.conan/data/libpq/14.5/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/mongo-c-driver/1.22.0/_/_/package/122c7fdd6ac5f79d4d69de394058fec92c456b93/lib" "/Users/alexiprof/.conan/data/hiredis/1.0.2/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/amqp-cpp/4.3.16/_/_/package/9955bffe79339c531342a3a20c0a10a1e5a3a2e0/lib" "/Users/alexiprof/.conan/data/gtest/1.12.1/_/_/package/57ad28a0cf35b24333da975af4fc357fd82c04ec/lib" "/Users/alexiprof/.conan/data/benchmark/1.6.2/_/_/package/4939359d58984e7dd11a2299dc27409aada19471/lib" "/Users/alexiprof/.conan/data/bzip2/1.0.8/_/_/package/4d994402bb6d6e8babdc89ce439cd1a0ab9877cd/lib" "/Users/alexiprof/.conan/data/libbacktrace/cci.20210118/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/libiconv/1.17/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/lib" "/Users/alexiprof/.conan/data/abseil/20220623.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/lib" "/Users/alexiprof/.conan/data/re2/20220601/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/lib" "/Users/alexiprof/.conan/data/protobuf/3.21.4/_/_/package/8264a6a82f1a58ddbf1d543f69f549b8e37e8e6a/lib" "/Users/alexiprof/.conan/data/googleapis/cci.20221108/_/_/package/36d26bdbe5ab4d99216fb52192a2302bff17b970/lib" "/Users/alexiprof/.conan/data/grpc-proto/cci.20220627/_/_/package/4b7ed5be66f388b91704f85ba054bc3a1947e5f0/lib" "/Users/alexiprof/.conan/data/cyrus-sasl/2.1.27/_/_/package/7e7af0ed7931a161b325bff1e2f6c62e8ff7cc0a/lib" "/Users/alexiprof/.conan/data/snappy/1.1.10/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/lib" "/Users/alexiprof/.conan/data/zstd/1.5.4/_/_/package/20e720aadae4b384dc7d39361f6ec68b8253a23a/lib" "/Users/alexiprof/.conan/data/icu/72.1/_/_/package/f377eb916255bfd558ed1b3ba2eb6e71aad6301e/lib")
list(PREPEND CMAKE_FRAMEWORK_PATH "/Users/alexiprof/.conan/data/spdlog/1.9.0/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/Frameworks" "/Users/alexiprof/.conan/data/grpc-proto/cci.20220627/_/_/package/4b7ed5be66f388b91704f85ba054bc3a1947e5f0/Frameworks")
list(PREPEND CMAKE_INCLUDE_PATH "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/rabbitmq" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/redis" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/mongo" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/postgresql" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/utest" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/api-common-protos" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/grpc" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/core" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/shared" "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e/include/universal" "/Users/alexiprof/.conan/data/boost/1.79.0/_/_/package/effc61087c57d5292f5fa96377e7b5e2c60734c9/include" "/Users/alexiprof/.conan/data/c-ares/1.18.1/_/_/package/1b6931ff6fdc18b784cabfbe6f3b36facd6c1cd9/include" "/Users/alexiprof/.conan/data/cctz/2.3/_/_/package/6f1931e5860f734d55456a9298992b6b0b094a59/include" "/Users/alexiprof/.conan/data/concurrentqueue/1.0.3/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/include" "/Users/alexiprof/.conan/data/concurrentqueue/1.0.3/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/include/moodycamel" "/Users/alexiprof/.conan/data/cryptopp/8.7.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/include" "/Users/alexiprof/.conan/data/fmt/8.1.1/_/_/package/80138d4a58def120da0b8c9199f2b7a4e464a85b/include" "/Users/alexiprof/.conan/data/libnghttp2/1.51.0/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/libcurl/7.86.0/_/_/package/a097455223234e250d01a2687cf7c15446fbd5d5/include" "/Users/alexiprof/.conan/data/libev/4.33/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/http_parser/2.9.4/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/openssl/1.1.1s/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/rapidjson/cci.20220822/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/include" "/Users/alexiprof/.conan/data/spdlog/1.9.0/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9/include" "/Users/alexiprof/.conan/data/yaml-cpp/0.7.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/include" "/Users/alexiprof/.conan/data/zlib/1.2.13/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/jemalloc/5.2.1/_/_/package/63ef65eed81da45031b3c1f57398571a636e3b31/include" "/Users/alexiprof/.conan/data/jemalloc/5.2.1/_/_/package/63ef65eed81da45031b3c1f57398571a636e3b31/include/jemalloc" "/Users/alexiprof/.conan/data/grpc/1.48.0/_/_/package/747f375e854e326b233e80494b3c63476474d77a/include" "/Users/alexiprof/.conan/data/libpq/14.5/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/mongo-c-driver/1.22.0/_/_/package/122c7fdd6ac5f79d4d69de394058fec92c456b93/include/libmongoc-1.0" "/Users/alexiprof/.conan/data/mongo-c-driver/1.22.0/_/_/package/122c7fdd6ac5f79d4d69de394058fec92c456b93/include/libbson-1.0" "/Users/alexiprof/.conan/data/hiredis/1.0.2/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/amqp-cpp/4.3.16/_/_/package/9955bffe79339c531342a3a20c0a10a1e5a3a2e0/include" "/Users/alexiprof/.conan/data/gtest/1.12.1/_/_/package/57ad28a0cf35b24333da975af4fc357fd82c04ec/include" "/Users/alexiprof/.conan/data/benchmark/1.6.2/_/_/package/4939359d58984e7dd11a2299dc27409aada19471/include" "/Users/alexiprof/.conan/data/bzip2/1.0.8/_/_/package/4d994402bb6d6e8babdc89ce439cd1a0ab9877cd/include" "/Users/alexiprof/.conan/data/libbacktrace/cci.20210118/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/libiconv/1.17/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d/include" "/Users/alexiprof/.conan/data/abseil/20220623.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/include" "/Users/alexiprof/.conan/data/re2/20220601/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/include" "/Users/alexiprof/.conan/data/protobuf/3.21.4/_/_/package/8264a6a82f1a58ddbf1d543f69f549b8e37e8e6a/include" "/Users/alexiprof/.conan/data/googleapis/cci.20221108/_/_/package/36d26bdbe5ab4d99216fb52192a2302bff17b970/include" "/Users/alexiprof/.conan/data/grpc-proto/cci.20220627/_/_/package/4b7ed5be66f388b91704f85ba054bc3a1947e5f0/include" "/Users/alexiprof/.conan/data/cyrus-sasl/2.1.27/_/_/package/7e7af0ed7931a161b325bff1e2f6c62e8ff7cc0a/include" "/Users/alexiprof/.conan/data/snappy/1.1.10/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458/include" "/Users/alexiprof/.conan/data/zstd/1.5.4/_/_/package/20e720aadae4b384dc7d39361f6ec68b8253a23a/include" "/Users/alexiprof/.conan/data/icu/72.1/_/_/package/f377eb916255bfd558ed1b3ba2eb6e71aad6301e/include")



if (DEFINED ENV{PKG_CONFIG_PATH})
set(ENV{PKG_CONFIG_PATH} "/Users/alexiprof/opt/userver/conan/test_package/build/Release/generators:$ENV{PKG_CONFIG_PATH}")
else()
set(ENV{PKG_CONFIG_PATH} "/Users/alexiprof/opt/userver/conan/test_package/build/Release/generators:")
endif()




# Variables
# Variables  per configuration


# Preprocessor definitions
# Preprocessor definitions per configuration
