project(userver-redis CXX)

userver_module(redis
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"

    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    UTEST_LINK_LIBRARIES ${PROJECT_NAME}-utest
    DBTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_redistest.cpp"
    DBTEST_DATABASES redis redis-cluster
    UBENCH_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark"
    UBENCH_DATABASES redis
)

# for libev
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/redis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/sentinel_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/subscription_rebalance_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/ev_thread/thread.cpp
    PROPERTIES COMPILE_FLAGS -fno-strict-aliasing
)
target_compile_options (${PROJECT_NAME} PRIVATE -Wno-old-style-cast)

option(USERVER_FEATURE_REDIS_HI_MALLOC "Provide 'hi_malloc(unsigned long)' function to workaround https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025" OFF)
if (USERVER_FEATURE_REDIS_HI_MALLOC)
  set_source_files_properties(
      ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/hi_malloc.cpp
      PROPERTIES COMPILE_FLAGS -DUSERVER_FEATURE_REDIS_HI_MALLOC=1
  )
endif()

option(USERVER_FEATURE_REDIS_TLS "SSL/TLS support for Redis driver" OFF)
if(USERVER_FEATURE_REDIS_TLS)
  add_compile_definitions(USERVER_FEATURE_REDIS_TLS=1)
endif()

if (USERVER_CONAN)
  find_package(hiredis REQUIRED)
  target_link_libraries(${PROJECT_NAME} PUBLIC hiredis::hiredis)
else()
  find_package(Hiredis REQUIRED)
  target_link_libraries(${PROJECT_NAME} PUBLIC Hiredis)
endif()

if (USERVER_FEATURE_REDIS_TLS)
  target_link_libraries(${PROJECT_NAME} PUBLIC hiredis_ssl)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)

_userver_directory_install(COMPONENT redis FILES
	  "${USERVER_ROOT_DIR}/cmake/modules/FindHiredis.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

# Helper library for testing user code with redis. Provides mock_client_base.hpp
# and other helper headers
if (USERVER_FEATURE_UTEST)
  add_subdirectory(testing)
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(functional_tests)
endif()
