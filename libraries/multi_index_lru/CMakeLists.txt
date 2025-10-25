project(userver-multi-index-lru CXX)

find_package(Boost REQUIRED) 


if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/multi_index_lru.cpp")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/multi_index_lru.cpp"
        "// MultiIndex LRU header-only lib\n"
        "// This file exists only to satisfy CMake target requirements\n"
        "namespace userver::lru_boost_list { const char* multi_index_lru_version = \"1.0\"; }\n")
endif()

userver_module(
    multi-index-lru
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp"
    DEPENDS core
)

target_include_directories(userver-multi-index-lru
    PUBLIC ${Boost_INCLUDE_DIRS}
)

_userver_directory_install(
    COMPONENT multi-index-lru
    FILES "${USERVER_ROOT_DIR}/cmake/modules/Findboost.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

# if(USERVER_FEATURE_UTEST)
    add_subdirectory(tests)
# endif()