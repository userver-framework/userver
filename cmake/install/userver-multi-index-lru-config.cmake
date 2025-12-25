include_guard(GLOBAL)

if(userver_multi_index_lru_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(Boost REQUIRED)

set(userver_multi_index_lru_FOUND TRUE)
