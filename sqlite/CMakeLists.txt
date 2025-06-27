project(userver-sqlite CXX)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Sqlite3 version from MacOS Mono framework is incompatible with userver driver
    set(USERVER_IMPL_CMAKE_FIND_FRAMEWORK_OLD ${CMAKE_FIND_FRAMEWORK})
    set(CMAKE_FIND_FRAMEWORK NEVER)

    find_package(SQLite3 REQUIRED)

    set(CMAKE_FIND_FRAMEWORK ${USERVER_IMPL_CMAKE_FIND_FRAMEWORK_OLD})
    unset(USERVER_IMPL_CMAKE_FIND_FRAMEWORK_OLD)
else()
    find_package(SQLite3 REQUIRED)
endif()

# Perhaps with this option it will be work faster, but should guarantee that the connection is used at the same time
# only one thread https://www.sqlite.org/threadsafe.html
set(SQLITE_THREADSAFE
    2
    CACHE
        STRING
        "​The SQLITE_THREADSAFE parameter configures SQLite's threading mode, determining how it handles concurrent operations. The available settings are:​:
0 (Single-thread): Disables all mutexes, making SQLite unsafe for use in more than a single thread at once.
1 (Serialized): Allows SQLite to be safely used by multiple threads with no restrictions.
2 (Multi-thread): Permits SQLite to be safely used by multiple threads, provided that no single database connection is used simultaneously in two or more threads."
)

userver_module(
    sqlite
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE SQLite::SQLite3
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

if(USERVER_BUILD_TESTS)
    add_subdirectory(functional_tests)
endif()
