include_guard(GLOBAL)

if(userver_rabbitmq_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

if(USERVER_CONAN)
  find_package(amqpcpp REQUIRED CONFIG)
else()
  find_package(amqpcpp REQUIRED)
endif()

set(userver_rabbitmq_FOUND TRUE)
