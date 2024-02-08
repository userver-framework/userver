include_guard(GLOBAL)

function(_make_stacktrace_target TARGET Boost_VERSION_STRING)
  # You can choose the implementation depending on your needs.
  # See http://boostorg.github.io/stacktrace/stacktrace/configuration_and_build.html for more info.
  option(USERVER_FEATURE_STACKTRACE "Allow capturing stacktraces using boost::stacktrace" ON)

  if(USERVER_FEATURE_STACKTRACE AND "${Boost_VERSION_STRING}" VERSION_LESS "1.69.0")
    # See https://github.com/boostorg/stacktrace/commit/4123beb4af6ff4e36769905b87c206da39190847
    message(WARNING
        "Pre-1.69 Boost.Stacktrace consumes a lot of memory and "
        "decodes stack slowly. Disabling stacktrace support."
    )
    set(USERVER_FEATURE_STACKTRACE OFF)
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "Darwin" OR "${CMAKE_SYSTEM}" MATCHES "BSD")
    set(USERVER_FEATURE_STACKTRACE OFF)
  endif()

  add_library(userver-stacktrace INTERFACE)
  if(USERVER_FEATURE_STACKTRACE)
    target_link_libraries("${TARGET}" INTERFACE Boost::stacktrace_backtrace backtrace dl)
  else()
    target_link_libraries("${TARGET}" INTERFACE Boost::stacktrace_basic dl)
  endif()
  target_compile_definitions("${TARGET}" INTERFACE BOOST_STACKTRACE_LINK)
endfunction()
