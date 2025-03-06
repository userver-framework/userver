project(userver-sqlite CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
file(GLOB_RECURSE SQLITE_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp
)

file(GLOB_RECURSE SQLITE_FUNCTIONAL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*
)
list(REMOVE_ITEM SOURCES ${SQLITE_FUNCTIONAL_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core)

if (USERVER_CONAN)
  find_package(SQLite3 REQUIRED)
  target_link_libraries(${PROJECT_NAME} PUBLIC SQLite::SQLite3)
else()
  find_package(SQLite3 REQUIRED)
  if (SQLITE3_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC SQLite::SQLite3)
  endif (SQLITE3_FOUND)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

_userver_directory_install(COMPONENT sqlite
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT sqlite TARGETS ${PROJECT_NAME})

_userver_directory_install(COMPONENT sqlite FILES
    ${USERVER_ROOT_DIR}/cmake/install/userver-sqlite3-config.cmake
    ${USERVER_ROOT_DIR}/cmake/modules/FindSQLite.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}-unittest ${SQLITE_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}-unittest PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-unittest PRIVATE
      userver-utest
      ${PROJECT_NAME}
  )
  add_google_tests(${PROJECT_NAME}-unittest)

  add_subdirectory(functional_tests)
endif()
