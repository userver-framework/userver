find_library(CRYPTO_LIBRARIES NAMES crypto)
mark_as_advanced(CRYPTO_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set MONGOCLIENT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Crypto REQUIRED_VARS CRYPTO_LIBRARIES)
