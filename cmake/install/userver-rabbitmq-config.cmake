include_guard(GLOBAL)

if(userver_rabbitmq_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/SetupAmqpCPP.cmake")

set(userver_rabbitmq_FOUND TRUE)
