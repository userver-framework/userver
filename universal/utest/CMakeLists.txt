project(userver-universal-utest CXX)

if(USERVER_CONAN)
  find_package(GTest REQUIRED CONFIG)
else()
  include(SetupGTest)
endif()

file(GLOB_RECURSE UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

add_library(userver-universal-utest-base STATIC ${SOURCES})
target_link_libraries(userver-universal-utest-base
    PUBLIC
    userver-universal
    GTest::gtest
    GTest::gmock
)
target_include_directories(userver-universal-utest-base
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
    $<TARGET_PROPERTY:userver-universal,INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
_userver_directory_install(COMPONENT universal
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT universal
    TARGETS userver-universal-utest-base
)

add_library(userver-universal-utest INTERFACE)
target_link_libraries(userver-universal-utest INTERFACE
    userver-universal-utest-base
    GTest::gmock_main
)
_userver_install_targets(COMPONENT universal
    TARGETS userver-universal-utest
)

if(USERVER_BUILD_TESTS)
  add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}-unittest SYSTEM PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME}-base,INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-unittest PUBLIC ${PROJECT_NAME})
  add_google_tests(${PROJECT_NAME}-unittest)
endif()
