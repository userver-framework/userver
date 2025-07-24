include_guard(GLOBAL)

function(userver_module MODULE)
    unset(ARG_UNPARSED_ARGUMENTS)
    set(OPTIONS NO_INSTALL NO_CORE_LINK GENERATE_DYNAMIC_CONFIGS)
    set(ONE_VALUE_ARGS SOURCE_DIR INSTALL_COMPONENT)
    set(MULTI_VALUE_ARGS
        IGNORE_SOURCES
        LINK_LIBRARIES
        LINK_LIBRARIES_PRIVATE
        INCLUDE_DIRS
        INCLUDE_DIRS_PRIVATE
        UTEST_DIRS
        UTEST_SOURCES
        UTEST_LINK_LIBRARIES
        DBTEST_DIRS
        DBTEST_SOURCES
        DBTEST_LINK_LIBRARIES
        DBTEST_DATABASES
        DBTEST_ENV
        UBENCH_DIRS
        UBENCH_SOURCES
        UBENCH_LINK_LIBRARIES
        UBENCH_DATABASES
        UBENCH_ENV
    )
    cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    # 1. userver-${MODULE}
    file(GLOB_RECURSE SOURCES "${ARG_SOURCE_DIR}/src/*.cpp" "${ARG_SOURCE_DIR}/src/*.hpp"
         "${ARG_SOURCE_DIR}/include/*.hpp"
    )
    # cmake <= 3.19 has a bug in remove_item
    list(REMOVE_ITEM SOURCES ${ARG_IGNORE_SOURCES} "")

    list(TRANSFORM ARG_UTEST_DIRS APPEND "/*.cpp" OUTPUT_VARIABLE UTEST_DIRS_SOURCES)
    file(GLOB_RECURSE UTEST_SOURCES ${ARG_UTEST_SOURCES} ${UTEST_DIRS_SOURCES})
    list(REMOVE_ITEM SOURCES ${UTEST_SOURCES} "")

    list(TRANSFORM ARG_DBTEST_DIRS APPEND "/*.cpp" OUTPUT_VARIABLE DBTEST_DIRS_SOURCES)
    file(GLOB_RECURSE DBTEST_SOURCES ${ARG_DBTEST_SOURCES} ${DBTEST_DIRS_SOURCES})
    list(REMOVE_ITEM SOURCES ${DBTEST_SOURCES} "")

    list(TRANSFORM ARG_UBENCH_DIRS APPEND "/*.cpp" OUTPUT_VARIABLE UBENCH_DIRS_SOURCES)
    file(GLOB_RECURSE UBENCH_SOURCES ${ARG_UBENCH_SOURCES} ${UBENCH_DIRS_SOURCES})
    list(REMOVE_ITEM SOURCES ${UBENCH_SOURCES} "")

    add_library(userver-${MODULE} STATIC ${SOURCES})
    set_target_properties(userver-${MODULE} PROPERTIES LINKER_LANGUAGE CXX)

    target_include_directories(
        userver-${MODULE}
        PUBLIC $<BUILD_INTERFACE:${ARG_SOURCE_DIR}/include> ${ARG_INCLUDE_DIRS}
        PRIVATE "${ARG_SOURCE_DIR}/src" ${ARG_INCLUDE_DIRS_PRIVATE}
    )
    target_link_libraries(
        userver-${MODULE}
        PUBLIC ${ARG_LINK_LIBRARIES}
        PRIVATE ${ARG_LINK_LIBRARIES_PRIVATE}
    )
    if(NOT ARG_NO_CORE_LINK)
        target_link_libraries(userver-${MODULE} PUBLIC userver-core)
    endif()

    if(NOT ARG_NO_INSTALL)
        if(ARG_INSTALL_COMPONENT)
            set(INSTALL_COMPONENT ${ARG_INSTALL_COMPONENT})
        else()
            set(INSTALL_COMPONENT ${MODULE})
        endif()
        _userver_directory_install(
            COMPONENT ${INSTALL_COMPONENT}
            DIRECTORY "${ARG_SOURCE_DIR}/include"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
        )
        _userver_install_targets(COMPONENT ${INSTALL_COMPONENT} TARGETS userver-${MODULE})
        if(NOT ARG_INSTALL_COMPONENT)
            set(install_config_file "${USERVER_ROOT_DIR}/cmake/install/userver-${MODULE}-config.cmake")
            if(NOT EXISTS ${install_config_file})
                message(FATAL_ERROR "Can not install ${MODULE}, no installation config in ${install_config_file}")
            endif()

            _userver_directory_install(
                COMPONENT ${MODULE}
                FILES "${install_config_file}"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
            )
        endif()
    endif()

    # 1. userver-${MODULE}-unittest
    if(USERVER_BUILD_TESTS AND UTEST_SOURCES)
        add_executable(userver-${MODULE}-unittest ${UTEST_SOURCES})
        target_link_libraries(
            userver-${MODULE}-unittest PRIVATE userver-utest userver-${MODULE} ${ARG_UTEST_LINK_LIBRARIES}
        )
        target_include_directories(
            userver-${MODULE}-unittest PRIVATE $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
                                               ${ARG_UTEST_DIRS}
        )
        add_google_tests(userver-${MODULE}-unittest)
    endif()

    # 1. userver-${MODULE}-dbtest
    if(USERVER_BUILD_TESTS AND DBTEST_SOURCES)
        add_executable(userver-${MODULE}-dbtest ${DBTEST_SOURCES})
        target_link_libraries(
            userver-${MODULE}-dbtest PRIVATE userver-utest userver-${MODULE} ${ARG_DBTEST_LINK_LIBRARIES}
        )
        target_include_directories(
            userver-${MODULE}-dbtest PRIVATE $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
                                             ${ARG_DBTEST_DIRS}
        )
        userver_add_utest(NAME userver-${MODULE}-dbtest DATABASES ${ARG_DBTEST_DATABASES} TEST_ENV ${ARG_DBTEST_ENV})
    endif()

    # 1. userver-${MODULE}-benchmark
    if(USERVER_BUILD_TESTS AND UBENCH_SOURCES)
        add_executable(userver-${MODULE}-benchmark ${UBENCH_SOURCES})
        target_link_libraries(
            userver-${MODULE}-benchmark PRIVATE userver-ubench userver-${MODULE} ${ARG_UBENCH_LINK_LIBRARIES}
        )
        target_include_directories(
            userver-${MODULE}-benchmark PRIVATE $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
                                                ${ARG_UBENCH_DIRS}
        )
        userver_add_ubench_test(
            NAME userver-${MODULE}-benchmark DATABASES ${ARG_UBENCH_DATABASES} TEST_ENV ${ARG_UBENCH_ENV}
        )
    endif()

    # 1. userver-${MODULE}-dynamic-configs
    if(ARG_GENERATE_DYNAMIC_CONFIGS)
        userver_target_generate_chaotic_dynamic_configs(userver-${MODULE}-dynamic-configs dynamic_configs/*.yaml)
        target_link_libraries(userver-${MODULE} PUBLIC userver-${MODULE}-dynamic-configs)
        _userver_install_targets(
	    COMPONENT ${MODULE}
	    TARGETS userver-${MODULE}-dynamic-configs
        )
        _userver_directory_install(
            COMPONENT ${MODULE}
            DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dynamic_configs/include
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
        )
    endif()
endfunction()
