find_path(MONGOCLIENT_INCLUDE_DIR mongo/version.h)
find_library(MONGOCLIENT_LIBRARIES NAMES libmongoclient.a)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS regex system thread)

set(MONGOCLIENT_LIBRARIES
  ${MONGOCLIENT_LIBRARIES}
  ${OPENSSL_CRYPTO_LIBRARY}
  ${OPENSSL_SSL_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  MongoClient DEFAULT_MSG MONGOCLIENT_LIBRARIES MONGOCLIENT_INCLUDE_DIR)
mark_as_advanced(MONGOCLIENT_LIBRARIES MONGOCLIENT_INCLUDE_DIR)

if(NOT MONGOCLIENT_FOUND)
	message(FATAL_ERROR "Cannot find development files for the mongoclient library. Please install libyandex-mongo-legacy-cxx-driver-dev.")
endif()
