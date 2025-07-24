include_guard(GLOBAL)

function(userver_embed_file TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS NAME FILEPATH HPP_FILENAME)
    set(MULTI_VALUE_ARGS SQL_FILES)
    cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})
    if(NOT ARG_HPP_FILENAME)
        get_filename_component(ARG_HPP_FILENAME "${ARG_FILEPATH}" NAME)
    endif()

    get_filename_component(ARG_FILEPATH "${ARG_FILEPATH}" REALPATH)
    set(CONFIG_HPP ${CMAKE_CURRENT_BINARY_DIR}/embedded/include/generated/${ARG_HPP_FILENAME}.hpp)

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/embedded/embedded.cpp
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/embedded && touch
                ${CMAKE_CURRENT_BINARY_DIR}/embedded/embedded.cpp ${CODEGEN}
    )
    _userver_codegen_register_files("${CMAKE_CURRENT_BINARY_DIR}/embedded/embedded.cpp")

    get_property(USERVER_CMAKE_DIR GLOBAL PROPERTY userver_cmake_dir)
    add_custom_command(
        OUTPUT ${CONFIG_HPP}
        DEPENDS ${USERVER_CMAKE_DIR}/embedded_config.cmake ${ARG_FILEPATH}
        COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DFILEPATH=${ARG_FILEPATH}
                -DOUTPUT=${CONFIG_HPP} -DNAME=${ARG_NAME} -P ${USERVER_CMAKE_DIR}/embedded_config.cmake ${CODEGEN}
    )
    _userver_codegen_register_files("${CONFIG_HPP}")
    add_library(${TARGET} STATIC ${CONFIG_HPP} ${CMAKE_CURRENT_BINARY_DIR}/embedded/embedded.cpp)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/embedded/include)
    target_link_libraries(${TARGET} PUBLIC userver::universal)
endfunction()
