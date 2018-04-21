cmake_minimum_required(VERSION 3.5)

set(PROJECT yandex-taxi-driver-authorizer)
project(${PROJECT} VERSION ${DEBVERSION})

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

find_package(JsonCpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(${PROJECT} ${SOURCES})
add_dependencies(${PROJECT} yandex-taxi-userver)
set_target_properties(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT}
  yandex-taxi-userver
  ${JSONCPP_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${CLANG_LIBRARIES}
)

target_include_directories(${PROJECT} PRIVATE
  ${JSONCPP_INCLUDE_DIR}
)


set(DEV_CONFIGS
  config_dev.json
  config_vars.development.json
  secdist_dev.json
  taxi_config_fallback.development.json)

include(PrependPaths)
prepend_paths(DEV_CONFIGS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/configs ${DEV_CONFIGS})
prepend_paths(DEV_CONFIGS_DEST ${CMAKE_CURRENT_BINARY_DIR} ${DEV_CONFIGS})
add_custom_target(dev_configs ALL DEPENDS ${DEV_CONFIGS_DEST})
add_custom_command(OUTPUT ${DEV_CONFIGS_DEST}
  COMMENT "Copying dev configs"
  COMMAND cp ${DEV_CONFIGS_SRC} ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${DEV_CONFIGS_SRC}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  CONFIGURATIONS Release)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/configs/
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/yandex/taxi/driver-authorizer
  CONFIGURATIONS Release
  PATTERN "*_dev.json" EXCLUDE)
