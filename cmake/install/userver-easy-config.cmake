include_guard(GLOBAL)

if(userver_easy_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS postgresql)

set(userver_easy_FOUND TRUE)
