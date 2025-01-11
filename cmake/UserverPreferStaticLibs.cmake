include_guard(GLOBAL)

if(USERVER_USE_STATIC_LIBS)
  set(OPENSSL_USE_STATIC_LIBS ON)
  set(ZLIB_USE_STATIC_LIBS ON)
  set(Protobuf_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_LIBS ON)
  set(CURL_USE_STATIC_LIBS ON)

  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")
  if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dylib")
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".tbd")
  endif()

  set(_FIND_LIB_PRIORITY_MESSAGE "Static libraries are prefered")
else()
  set(_FIND_LIB_PRIORITY_MESSAGE "Default ordered")
endif()

message(STATUS "Modules type search priority order: ${CMAKE_FIND_LIBRARY_SUFFIXES} (${_FIND_LIB_PRIORITY_MESSAGE})")
