find_package(MongoClient REQUIRED)
find_package(JsonCpp REQUIRED)

find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  exec_program(${CPPCHECK} ARGS --version OUTPUT_VARIABLE CPPCHECK_OUTPUT)
  string(REGEX REPLACE "Cppcheck [0-9]*.([0-9.]*)" "\\1" CPPCHECK_MINOR "${CPPCHECK_OUTPUT}")
  message(STATUS "cppcheck: ${CPPCHECK} 1.${CPPCHECK_MINOR}")
else()
  message(STATUS "cppcheck: not found (install cppcheck package)")
endif()


if (${CPPCHECK_MINOR} GREATER 76)
  add_custom_target(
  cppcheck
  COMMAND ${CPPCHECK}
          --enable=warning,performance,portability,missingInclude
          --std=c++14
          --language=c++
          --template="[{severity}][{id}] {message} {callstack} \(On\ {file}:{line}\)"
          --verbose
          --quiet
          --force
          --project=${CMAKE_BINARY_DIR}/compile_commands.json
  )
else()
  file(GLOB_RECURSE ALL_SOURCE_FILES ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.hpp)
  add_custom_target(
  cppcheck
  COMMAND ${CPPCHECK}
          --enable=warning,performance,portability,missingInclude
          --std=c++11 # c++14 is not available :(
          --language=c++
          --template="[{severity}][{id}] {message} {callstack} \(On\ {file}:{line}\)"
          --verbose
          --quiet
          --force
          -I ${USERVER_THIRD_PARTY_DIRS}
          -I ${MONGOCLIENT_INCLUDE_DIR}
          -I ${JSONCPP_INCLUDE_DIR}
          ${ALL_SOURCE_FILES}
  )
endif()
