include_guard(GLOBAL)

if(userver_rabbitmq_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(amqpcpp REQUIRED)

set(userver_rabbitmq_FOUND TRUE)
