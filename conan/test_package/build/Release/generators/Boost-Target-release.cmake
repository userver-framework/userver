# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(boost_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(boost_FRAMEWORKS_FOUND_RELEASE "${boost_FRAMEWORKS_RELEASE}" "${boost_FRAMEWORK_DIRS_RELEASE}")

set(boost_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET boost_DEPS_TARGET)
    add_library(boost_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET boost_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${boost_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${boost_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:Boost::diagnostic_definitions;Boost::disable_autolinking;Boost::dynamic_linking;Boost::headers;Boost::headers;boost::_libboost;Boost::system;boost::_libboost;boost::_libboost;boost::_libboost;Boost::exception;Boost::thread;boost::_libboost;Boost::context;Boost::exception;Boost::system;boost::_libboost;boost::_libboost;boost::_libboost;Boost::atomic;Boost::system;boost::_libboost;BZip2::BZip2;ZLIB::ZLIB;Boost::random;Boost::regex;boost::_libboost;Iconv::Iconv;Boost::thread;boost::_libboost;Boost::atomic;Boost::container;Boost::date_time;Boost::exception;Boost::filesystem;Boost::random;Boost::regex;Boost::system;Boost::thread;boost::_libboost;Boost::log;boost::_libboost;Boost::test;boost::_libboost;boost::_libboost;Boost::system;boost::_libboost;boost::_libboost;boost::_libboost;boost::_libboost;Boost::stacktrace;boost::_libboost;libbacktrace::libbacktrace;Boost::stacktrace;boost::_libboost;Boost::stacktrace;boost::_libboost;Boost::stacktrace;boost::_libboost;boost::_libboost;Boost::exception;boost::_libboost;Boost::test;boost::_libboost;Boost::atomic;Boost::chrono;Boost::container;Boost::date_time;Boost::exception;Boost::system;boost::_libboost;Boost::chrono;Boost::system;boost::_libboost;Boost::thread;boost::_libboost;Boost::prg_exec_monitor;Boost::test;Boost::test_exec_monitor;boost::_libboost;Boost::serialization;boost::_libboost>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### boost_DEPS_TARGET to all of them
conan_package_library_targets("${boost_LIBS_RELEASE}"    # libraries
                              "${boost_LIB_DIRS_RELEASE}" # package_libdir
                              boost_DEPS_TARGET
                              boost_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "boost")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${boost_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT Boost::log_setup #############

        set(boost_Boost_log_setup_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_log_setup_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_log_setup_FRAMEWORKS_RELEASE}" "${boost_Boost_log_setup_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_log_setup_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_log_setup_DEPS_TARGET)
            add_library(boost_Boost_log_setup_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_log_setup_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_log_setup_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_log_setup_LIBS_RELEASE}"
                                      "${boost_Boost_log_setup_LIB_DIRS_RELEASE}"
                                      boost_Boost_log_setup_DEPS_TARGET
                                      boost_Boost_log_setup_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_log_setup")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::log_setup
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_log_setup_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::log_setup
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_log_setup_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::log_setup PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::log_setup PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::log_setup PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::log_setup PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_log_setup_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::log #############

        set(boost_Boost_log_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_log_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_log_FRAMEWORKS_RELEASE}" "${boost_Boost_log_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_log_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_log_DEPS_TARGET)
            add_library(boost_Boost_log_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_log_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_log_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_log_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_log_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_log_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_log_LIBS_RELEASE}"
                                      "${boost_Boost_log_LIB_DIRS_RELEASE}"
                                      boost_Boost_log_DEPS_TARGET
                                      boost_Boost_log_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_log")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::log
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_log_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_log_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_log_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::log
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_log_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::log PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_log_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::log PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_log_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::log PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_log_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::log PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_log_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::locale #############

        set(boost_Boost_locale_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_locale_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_locale_FRAMEWORKS_RELEASE}" "${boost_Boost_locale_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_locale_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_locale_DEPS_TARGET)
            add_library(boost_Boost_locale_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_locale_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_locale_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_locale_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_locale_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_locale_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_locale_LIBS_RELEASE}"
                                      "${boost_Boost_locale_LIB_DIRS_RELEASE}"
                                      boost_Boost_locale_DEPS_TARGET
                                      boost_Boost_locale_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_locale")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::locale
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_locale_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_locale_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_locale_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::locale
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_locale_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::locale PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_locale_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::locale PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_locale_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::locale PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_locale_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::locale PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_locale_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::iostreams #############

        set(boost_Boost_iostreams_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_iostreams_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_iostreams_FRAMEWORKS_RELEASE}" "${boost_Boost_iostreams_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_iostreams_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_iostreams_DEPS_TARGET)
            add_library(boost_Boost_iostreams_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_iostreams_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_iostreams_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_iostreams_LIBS_RELEASE}"
                                      "${boost_Boost_iostreams_LIB_DIRS_RELEASE}"
                                      boost_Boost_iostreams_DEPS_TARGET
                                      boost_Boost_iostreams_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_iostreams")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::iostreams
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_iostreams_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::iostreams
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_iostreams_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::iostreams PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::iostreams PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::iostreams PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::iostreams PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_iostreams_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::contract #############

        set(boost_Boost_contract_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_contract_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_contract_FRAMEWORKS_RELEASE}" "${boost_Boost_contract_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_contract_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_contract_DEPS_TARGET)
            add_library(boost_Boost_contract_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_contract_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_contract_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_contract_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_contract_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_contract_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_contract_LIBS_RELEASE}"
                                      "${boost_Boost_contract_LIB_DIRS_RELEASE}"
                                      boost_Boost_contract_DEPS_TARGET
                                      boost_Boost_contract_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_contract")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::contract
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_contract_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_contract_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_contract_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::contract
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_contract_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::contract PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_contract_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::contract PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_contract_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::contract PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_contract_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::contract PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_contract_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::unit_test_framework #############

        set(boost_Boost_unit_test_framework_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_unit_test_framework_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_unit_test_framework_FRAMEWORKS_RELEASE}" "${boost_Boost_unit_test_framework_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_unit_test_framework_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_unit_test_framework_DEPS_TARGET)
            add_library(boost_Boost_unit_test_framework_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_unit_test_framework_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_unit_test_framework_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_unit_test_framework_LIBS_RELEASE}"
                                      "${boost_Boost_unit_test_framework_LIB_DIRS_RELEASE}"
                                      boost_Boost_unit_test_framework_DEPS_TARGET
                                      boost_Boost_unit_test_framework_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_unit_test_framework")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::unit_test_framework
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_unit_test_framework_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::unit_test_framework
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_unit_test_framework_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::unit_test_framework PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::unit_test_framework PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::unit_test_framework PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::unit_test_framework PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_unit_test_framework_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::type_erasure #############

        set(boost_Boost_type_erasure_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_type_erasure_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_type_erasure_FRAMEWORKS_RELEASE}" "${boost_Boost_type_erasure_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_type_erasure_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_type_erasure_DEPS_TARGET)
            add_library(boost_Boost_type_erasure_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_type_erasure_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_type_erasure_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_type_erasure_LIBS_RELEASE}"
                                      "${boost_Boost_type_erasure_LIB_DIRS_RELEASE}"
                                      boost_Boost_type_erasure_DEPS_TARGET
                                      boost_Boost_type_erasure_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_type_erasure")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::type_erasure
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_type_erasure_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::type_erasure
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_type_erasure_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::type_erasure PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::type_erasure PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::type_erasure PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::type_erasure PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_type_erasure_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::timer #############

        set(boost_Boost_timer_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_timer_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_timer_FRAMEWORKS_RELEASE}" "${boost_Boost_timer_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_timer_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_timer_DEPS_TARGET)
            add_library(boost_Boost_timer_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_timer_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_timer_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_timer_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_timer_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_timer_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_timer_LIBS_RELEASE}"
                                      "${boost_Boost_timer_LIB_DIRS_RELEASE}"
                                      boost_Boost_timer_DEPS_TARGET
                                      boost_Boost_timer_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_timer")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::timer
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_timer_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_timer_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_timer_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::timer
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_timer_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::timer PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_timer_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::timer PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_timer_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::timer PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_timer_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::timer PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_timer_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::thread #############

        set(boost_Boost_thread_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_thread_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_thread_FRAMEWORKS_RELEASE}" "${boost_Boost_thread_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_thread_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_thread_DEPS_TARGET)
            add_library(boost_Boost_thread_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_thread_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_thread_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_thread_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_thread_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_thread_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_thread_LIBS_RELEASE}"
                                      "${boost_Boost_thread_LIB_DIRS_RELEASE}"
                                      boost_Boost_thread_DEPS_TARGET
                                      boost_Boost_thread_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_thread")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::thread
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_thread_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_thread_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_thread_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::thread
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_thread_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::thread PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_thread_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::thread PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_thread_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::thread PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_thread_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::thread PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_thread_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::random #############

        set(boost_Boost_random_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_random_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_random_FRAMEWORKS_RELEASE}" "${boost_Boost_random_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_random_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_random_DEPS_TARGET)
            add_library(boost_Boost_random_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_random_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_random_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_random_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_random_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_random_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_random_LIBS_RELEASE}"
                                      "${boost_Boost_random_LIB_DIRS_RELEASE}"
                                      boost_Boost_random_DEPS_TARGET
                                      boost_Boost_random_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_random")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::random
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_random_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_random_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_random_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::random
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_random_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::random PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_random_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::random PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_random_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::random PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_random_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::random PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_random_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::prg_exec_monitor #############

        set(boost_Boost_prg_exec_monitor_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_prg_exec_monitor_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_prg_exec_monitor_FRAMEWORKS_RELEASE}" "${boost_Boost_prg_exec_monitor_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_prg_exec_monitor_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_prg_exec_monitor_DEPS_TARGET)
            add_library(boost_Boost_prg_exec_monitor_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_prg_exec_monitor_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_prg_exec_monitor_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_prg_exec_monitor_LIBS_RELEASE}"
                                      "${boost_Boost_prg_exec_monitor_LIB_DIRS_RELEASE}"
                                      boost_Boost_prg_exec_monitor_DEPS_TARGET
                                      boost_Boost_prg_exec_monitor_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_prg_exec_monitor")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::prg_exec_monitor
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_prg_exec_monitor_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::prg_exec_monitor
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_prg_exec_monitor_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::prg_exec_monitor PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::prg_exec_monitor PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::prg_exec_monitor PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::prg_exec_monitor PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_prg_exec_monitor_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::filesystem #############

        set(boost_Boost_filesystem_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_filesystem_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_filesystem_FRAMEWORKS_RELEASE}" "${boost_Boost_filesystem_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_filesystem_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_filesystem_DEPS_TARGET)
            add_library(boost_Boost_filesystem_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_filesystem_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_filesystem_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_filesystem_LIBS_RELEASE}"
                                      "${boost_Boost_filesystem_LIB_DIRS_RELEASE}"
                                      boost_Boost_filesystem_DEPS_TARGET
                                      boost_Boost_filesystem_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_filesystem")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::filesystem
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_filesystem_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::filesystem
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_filesystem_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::filesystem PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::filesystem PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::filesystem PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::filesystem PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_filesystem_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::coroutine #############

        set(boost_Boost_coroutine_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_coroutine_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_coroutine_FRAMEWORKS_RELEASE}" "${boost_Boost_coroutine_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_coroutine_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_coroutine_DEPS_TARGET)
            add_library(boost_Boost_coroutine_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_coroutine_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_coroutine_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_coroutine_LIBS_RELEASE}"
                                      "${boost_Boost_coroutine_LIB_DIRS_RELEASE}"
                                      boost_Boost_coroutine_DEPS_TARGET
                                      boost_Boost_coroutine_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_coroutine")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::coroutine
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_coroutine_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::coroutine
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_coroutine_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::coroutine PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::coroutine PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::coroutine PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::coroutine PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_coroutine_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::chrono #############

        set(boost_Boost_chrono_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_chrono_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_chrono_FRAMEWORKS_RELEASE}" "${boost_Boost_chrono_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_chrono_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_chrono_DEPS_TARGET)
            add_library(boost_Boost_chrono_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_chrono_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_chrono_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_chrono_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_chrono_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_chrono_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_chrono_LIBS_RELEASE}"
                                      "${boost_Boost_chrono_LIB_DIRS_RELEASE}"
                                      boost_Boost_chrono_DEPS_TARGET
                                      boost_Boost_chrono_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_chrono")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::chrono
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_chrono_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_chrono_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_chrono_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::chrono
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_chrono_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::chrono PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_chrono_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::chrono PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_chrono_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::chrono PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_chrono_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::chrono PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_chrono_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::wserialization #############

        set(boost_Boost_wserialization_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_wserialization_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_wserialization_FRAMEWORKS_RELEASE}" "${boost_Boost_wserialization_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_wserialization_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_wserialization_DEPS_TARGET)
            add_library(boost_Boost_wserialization_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_wserialization_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_wserialization_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_wserialization_LIBS_RELEASE}"
                                      "${boost_Boost_wserialization_LIB_DIRS_RELEASE}"
                                      boost_Boost_wserialization_DEPS_TARGET
                                      boost_Boost_wserialization_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_wserialization")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::wserialization
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_wserialization_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::wserialization
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_wserialization_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::wserialization PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::wserialization PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::wserialization PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::wserialization PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_wserialization_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::test_exec_monitor #############

        set(boost_Boost_test_exec_monitor_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_test_exec_monitor_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_test_exec_monitor_FRAMEWORKS_RELEASE}" "${boost_Boost_test_exec_monitor_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_test_exec_monitor_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_test_exec_monitor_DEPS_TARGET)
            add_library(boost_Boost_test_exec_monitor_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_test_exec_monitor_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_test_exec_monitor_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_test_exec_monitor_LIBS_RELEASE}"
                                      "${boost_Boost_test_exec_monitor_LIB_DIRS_RELEASE}"
                                      boost_Boost_test_exec_monitor_DEPS_TARGET
                                      boost_Boost_test_exec_monitor_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_test_exec_monitor")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::test_exec_monitor
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_test_exec_monitor_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::test_exec_monitor
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_test_exec_monitor_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::test_exec_monitor PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::test_exec_monitor PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::test_exec_monitor PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::test_exec_monitor PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_test_exec_monitor_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::test #############

        set(boost_Boost_test_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_test_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_test_FRAMEWORKS_RELEASE}" "${boost_Boost_test_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_test_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_test_DEPS_TARGET)
            add_library(boost_Boost_test_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_test_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_test_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_test_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_test_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_test_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_test_LIBS_RELEASE}"
                                      "${boost_Boost_test_LIB_DIRS_RELEASE}"
                                      boost_Boost_test_DEPS_TARGET
                                      boost_Boost_test_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_test")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::test
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_test_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_test_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_test_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::test
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_test_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::test PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_test_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::test PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_test_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::test PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_test_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::test PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_test_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::system #############

        set(boost_Boost_system_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_system_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_system_FRAMEWORKS_RELEASE}" "${boost_Boost_system_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_system_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_system_DEPS_TARGET)
            add_library(boost_Boost_system_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_system_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_system_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_system_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_system_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_system_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_system_LIBS_RELEASE}"
                                      "${boost_Boost_system_LIB_DIRS_RELEASE}"
                                      boost_Boost_system_DEPS_TARGET
                                      boost_Boost_system_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_system")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::system
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_system_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_system_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_system_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::system
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_system_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::system PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_system_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::system PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_system_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::system PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_system_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::system PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_system_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::stacktrace_noop #############

        set(boost_Boost_stacktrace_noop_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_stacktrace_noop_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_stacktrace_noop_FRAMEWORKS_RELEASE}" "${boost_Boost_stacktrace_noop_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_stacktrace_noop_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_stacktrace_noop_DEPS_TARGET)
            add_library(boost_Boost_stacktrace_noop_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_stacktrace_noop_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_stacktrace_noop_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_stacktrace_noop_LIBS_RELEASE}"
                                      "${boost_Boost_stacktrace_noop_LIB_DIRS_RELEASE}"
                                      boost_Boost_stacktrace_noop_DEPS_TARGET
                                      boost_Boost_stacktrace_noop_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_stacktrace_noop")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::stacktrace_noop
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_stacktrace_noop_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::stacktrace_noop
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_stacktrace_noop_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::stacktrace_noop PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_noop PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_noop PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_noop PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_noop_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::stacktrace_basic #############

        set(boost_Boost_stacktrace_basic_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_stacktrace_basic_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_stacktrace_basic_FRAMEWORKS_RELEASE}" "${boost_Boost_stacktrace_basic_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_stacktrace_basic_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_stacktrace_basic_DEPS_TARGET)
            add_library(boost_Boost_stacktrace_basic_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_stacktrace_basic_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_stacktrace_basic_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_stacktrace_basic_LIBS_RELEASE}"
                                      "${boost_Boost_stacktrace_basic_LIB_DIRS_RELEASE}"
                                      boost_Boost_stacktrace_basic_DEPS_TARGET
                                      boost_Boost_stacktrace_basic_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_stacktrace_basic")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::stacktrace_basic
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_stacktrace_basic_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::stacktrace_basic
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_stacktrace_basic_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::stacktrace_basic PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_basic PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_basic PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_basic PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_basic_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::stacktrace_backtrace #############

        set(boost_Boost_stacktrace_backtrace_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_stacktrace_backtrace_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_stacktrace_backtrace_FRAMEWORKS_RELEASE}" "${boost_Boost_stacktrace_backtrace_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_stacktrace_backtrace_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_stacktrace_backtrace_DEPS_TARGET)
            add_library(boost_Boost_stacktrace_backtrace_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_stacktrace_backtrace_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_stacktrace_backtrace_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_stacktrace_backtrace_LIBS_RELEASE}"
                                      "${boost_Boost_stacktrace_backtrace_LIB_DIRS_RELEASE}"
                                      boost_Boost_stacktrace_backtrace_DEPS_TARGET
                                      boost_Boost_stacktrace_backtrace_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_stacktrace_backtrace")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::stacktrace_backtrace
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_stacktrace_backtrace_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::stacktrace_backtrace
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_stacktrace_backtrace_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::stacktrace_backtrace PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_backtrace PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_backtrace PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_backtrace PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_backtrace_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::stacktrace_addr2line #############

        set(boost_Boost_stacktrace_addr2line_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_stacktrace_addr2line_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_stacktrace_addr2line_FRAMEWORKS_RELEASE}" "${boost_Boost_stacktrace_addr2line_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_stacktrace_addr2line_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_stacktrace_addr2line_DEPS_TARGET)
            add_library(boost_Boost_stacktrace_addr2line_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_stacktrace_addr2line_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_stacktrace_addr2line_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_stacktrace_addr2line_LIBS_RELEASE}"
                                      "${boost_Boost_stacktrace_addr2line_LIB_DIRS_RELEASE}"
                                      boost_Boost_stacktrace_addr2line_DEPS_TARGET
                                      boost_Boost_stacktrace_addr2line_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_stacktrace_addr2line")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::stacktrace_addr2line
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_stacktrace_addr2line_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::stacktrace_addr2line
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_stacktrace_addr2line_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::stacktrace_addr2line PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_addr2line PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_addr2line PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace_addr2line PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_addr2line_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::stacktrace #############

        set(boost_Boost_stacktrace_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_stacktrace_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_stacktrace_FRAMEWORKS_RELEASE}" "${boost_Boost_stacktrace_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_stacktrace_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_stacktrace_DEPS_TARGET)
            add_library(boost_Boost_stacktrace_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_stacktrace_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_stacktrace_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_stacktrace_LIBS_RELEASE}"
                                      "${boost_Boost_stacktrace_LIB_DIRS_RELEASE}"
                                      boost_Boost_stacktrace_DEPS_TARGET
                                      boost_Boost_stacktrace_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_stacktrace")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::stacktrace
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_stacktrace_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::stacktrace
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_stacktrace_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::stacktrace PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::stacktrace PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_stacktrace_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::serialization #############

        set(boost_Boost_serialization_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_serialization_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_serialization_FRAMEWORKS_RELEASE}" "${boost_Boost_serialization_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_serialization_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_serialization_DEPS_TARGET)
            add_library(boost_Boost_serialization_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_serialization_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_serialization_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_serialization_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_serialization_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_serialization_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_serialization_LIBS_RELEASE}"
                                      "${boost_Boost_serialization_LIB_DIRS_RELEASE}"
                                      boost_Boost_serialization_DEPS_TARGET
                                      boost_Boost_serialization_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_serialization")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::serialization
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_serialization_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_serialization_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_serialization_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::serialization
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_serialization_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::serialization PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_serialization_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::serialization PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_serialization_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::serialization PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_serialization_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::serialization PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_serialization_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::regex #############

        set(boost_Boost_regex_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_regex_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_regex_FRAMEWORKS_RELEASE}" "${boost_Boost_regex_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_regex_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_regex_DEPS_TARGET)
            add_library(boost_Boost_regex_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_regex_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_regex_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_regex_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_regex_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_regex_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_regex_LIBS_RELEASE}"
                                      "${boost_Boost_regex_LIB_DIRS_RELEASE}"
                                      boost_Boost_regex_DEPS_TARGET
                                      boost_Boost_regex_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_regex")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::regex
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_regex_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_regex_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_regex_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::regex
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_regex_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::regex PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_regex_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::regex PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_regex_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::regex PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_regex_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::regex PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_regex_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::program_options #############

        set(boost_Boost_program_options_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_program_options_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_program_options_FRAMEWORKS_RELEASE}" "${boost_Boost_program_options_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_program_options_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_program_options_DEPS_TARGET)
            add_library(boost_Boost_program_options_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_program_options_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_program_options_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_program_options_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_program_options_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_program_options_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_program_options_LIBS_RELEASE}"
                                      "${boost_Boost_program_options_LIB_DIRS_RELEASE}"
                                      boost_Boost_program_options_DEPS_TARGET
                                      boost_Boost_program_options_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_program_options")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::program_options
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_program_options_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_program_options_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_program_options_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::program_options
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_program_options_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::program_options PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_program_options_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::program_options PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_program_options_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::program_options PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_program_options_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::program_options PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_program_options_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::exception #############

        set(boost_Boost_exception_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_exception_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_exception_FRAMEWORKS_RELEASE}" "${boost_Boost_exception_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_exception_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_exception_DEPS_TARGET)
            add_library(boost_Boost_exception_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_exception_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_exception_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_exception_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_exception_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_exception_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_exception_LIBS_RELEASE}"
                                      "${boost_Boost_exception_LIB_DIRS_RELEASE}"
                                      boost_Boost_exception_DEPS_TARGET
                                      boost_Boost_exception_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_exception")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::exception
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_exception_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_exception_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_exception_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::exception
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_exception_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::exception PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_exception_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::exception PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_exception_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::exception PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_exception_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::exception PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_exception_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::date_time #############

        set(boost_Boost_date_time_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_date_time_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_date_time_FRAMEWORKS_RELEASE}" "${boost_Boost_date_time_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_date_time_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_date_time_DEPS_TARGET)
            add_library(boost_Boost_date_time_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_date_time_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_date_time_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_date_time_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_date_time_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_date_time_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_date_time_LIBS_RELEASE}"
                                      "${boost_Boost_date_time_LIB_DIRS_RELEASE}"
                                      boost_Boost_date_time_DEPS_TARGET
                                      boost_Boost_date_time_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_date_time")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::date_time
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_date_time_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_date_time_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_date_time_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::date_time
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_date_time_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::date_time PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_date_time_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::date_time PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_date_time_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::date_time PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_date_time_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::date_time PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_date_time_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::context #############

        set(boost_Boost_context_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_context_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_context_FRAMEWORKS_RELEASE}" "${boost_Boost_context_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_context_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_context_DEPS_TARGET)
            add_library(boost_Boost_context_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_context_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_context_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_context_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_context_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_context_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_context_LIBS_RELEASE}"
                                      "${boost_Boost_context_LIB_DIRS_RELEASE}"
                                      boost_Boost_context_DEPS_TARGET
                                      boost_Boost_context_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_context")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::context
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_context_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_context_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_context_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::context
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_context_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::context PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_context_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::context PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_context_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::context PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_context_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::context PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_context_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::container #############

        set(boost_Boost_container_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_container_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_container_FRAMEWORKS_RELEASE}" "${boost_Boost_container_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_container_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_container_DEPS_TARGET)
            add_library(boost_Boost_container_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_container_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_container_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_container_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_container_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_container_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_container_LIBS_RELEASE}"
                                      "${boost_Boost_container_LIB_DIRS_RELEASE}"
                                      boost_Boost_container_DEPS_TARGET
                                      boost_Boost_container_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_container")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::container
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_container_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_container_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_container_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::container
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_container_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::container PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_container_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::container PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_container_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::container PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_container_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::container PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_container_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::atomic #############

        set(boost_Boost_atomic_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_atomic_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_atomic_FRAMEWORKS_RELEASE}" "${boost_Boost_atomic_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_atomic_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_atomic_DEPS_TARGET)
            add_library(boost_Boost_atomic_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_atomic_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_atomic_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_atomic_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_atomic_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_atomic_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_atomic_LIBS_RELEASE}"
                                      "${boost_Boost_atomic_LIB_DIRS_RELEASE}"
                                      boost_Boost_atomic_DEPS_TARGET
                                      boost_Boost_atomic_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_atomic")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::atomic
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_atomic_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_atomic_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_atomic_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::atomic
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_atomic_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::atomic PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_atomic_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::atomic PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_atomic_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::atomic PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_atomic_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::atomic PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_atomic_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT boost::_libboost #############

        set(boost_boost__libboost_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_boost__libboost_FRAMEWORKS_FOUND_RELEASE "${boost_boost__libboost_FRAMEWORKS_RELEASE}" "${boost_boost__libboost_FRAMEWORK_DIRS_RELEASE}")

        set(boost_boost__libboost_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_boost__libboost_DEPS_TARGET)
            add_library(boost_boost__libboost_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_boost__libboost_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_boost__libboost_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_boost__libboost_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_boost__libboost_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_boost__libboost_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_boost__libboost_LIBS_RELEASE}"
                                      "${boost_boost__libboost_LIB_DIRS_RELEASE}"
                                      boost_boost__libboost_DEPS_TARGET
                                      boost_boost__libboost_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_boost__libboost")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET boost::_libboost
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_boost__libboost_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_boost__libboost_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_boost__libboost_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET boost::_libboost
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_boost__libboost_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET boost::_libboost PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_boost__libboost_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET boost::_libboost PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_boost__libboost_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET boost::_libboost PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_boost__libboost_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET boost::_libboost PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_boost__libboost_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::boost #############

        set(boost_Boost_boost_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_boost_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_boost_FRAMEWORKS_RELEASE}" "${boost_Boost_boost_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_boost_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_boost_DEPS_TARGET)
            add_library(boost_Boost_boost_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_boost_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_boost_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_boost_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_boost_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_boost_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_boost_LIBS_RELEASE}"
                                      "${boost_Boost_boost_LIB_DIRS_RELEASE}"
                                      boost_Boost_boost_DEPS_TARGET
                                      boost_Boost_boost_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_boost")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::boost
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_boost_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_boost_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_boost_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::boost
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_boost_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::boost PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_boost_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::boost PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_boost_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::boost PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_boost_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::boost PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_boost_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::headers #############

        set(boost_Boost_headers_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_headers_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_headers_FRAMEWORKS_RELEASE}" "${boost_Boost_headers_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_headers_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_headers_DEPS_TARGET)
            add_library(boost_Boost_headers_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_headers_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_headers_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_headers_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_headers_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_headers_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_headers_LIBS_RELEASE}"
                                      "${boost_Boost_headers_LIB_DIRS_RELEASE}"
                                      boost_Boost_headers_DEPS_TARGET
                                      boost_Boost_headers_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_headers")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::headers
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_headers_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_headers_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_headers_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::headers
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_headers_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::headers PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_headers_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::headers PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_headers_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::headers PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_headers_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::headers PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_headers_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::dynamic_linking #############

        set(boost_Boost_dynamic_linking_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_dynamic_linking_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_dynamic_linking_FRAMEWORKS_RELEASE}" "${boost_Boost_dynamic_linking_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_dynamic_linking_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_dynamic_linking_DEPS_TARGET)
            add_library(boost_Boost_dynamic_linking_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_dynamic_linking_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_dynamic_linking_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_dynamic_linking_LIBS_RELEASE}"
                                      "${boost_Boost_dynamic_linking_LIB_DIRS_RELEASE}"
                                      boost_Boost_dynamic_linking_DEPS_TARGET
                                      boost_Boost_dynamic_linking_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_dynamic_linking")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::dynamic_linking
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_dynamic_linking_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::dynamic_linking
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_dynamic_linking_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::dynamic_linking PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::dynamic_linking PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::dynamic_linking PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::dynamic_linking PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_dynamic_linking_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::disable_autolinking #############

        set(boost_Boost_disable_autolinking_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_disable_autolinking_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_disable_autolinking_FRAMEWORKS_RELEASE}" "${boost_Boost_disable_autolinking_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_disable_autolinking_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_disable_autolinking_DEPS_TARGET)
            add_library(boost_Boost_disable_autolinking_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_disable_autolinking_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_disable_autolinking_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_disable_autolinking_LIBS_RELEASE}"
                                      "${boost_Boost_disable_autolinking_LIB_DIRS_RELEASE}"
                                      boost_Boost_disable_autolinking_DEPS_TARGET
                                      boost_Boost_disable_autolinking_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_disable_autolinking")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::disable_autolinking
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_disable_autolinking_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::disable_autolinking
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_disable_autolinking_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::disable_autolinking PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::disable_autolinking PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::disable_autolinking PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::disable_autolinking PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_disable_autolinking_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT Boost::diagnostic_definitions #############

        set(boost_Boost_diagnostic_definitions_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(boost_Boost_diagnostic_definitions_FRAMEWORKS_FOUND_RELEASE "${boost_Boost_diagnostic_definitions_FRAMEWORKS_RELEASE}" "${boost_Boost_diagnostic_definitions_FRAMEWORK_DIRS_RELEASE}")

        set(boost_Boost_diagnostic_definitions_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET boost_Boost_diagnostic_definitions_DEPS_TARGET)
            add_library(boost_Boost_diagnostic_definitions_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET boost_Boost_diagnostic_definitions_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'boost_Boost_diagnostic_definitions_DEPS_TARGET' to all of them
        conan_package_library_targets("${boost_Boost_diagnostic_definitions_LIBS_RELEASE}"
                                      "${boost_Boost_diagnostic_definitions_LIB_DIRS_RELEASE}"
                                      boost_Boost_diagnostic_definitions_DEPS_TARGET
                                      boost_Boost_diagnostic_definitions_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "boost_Boost_diagnostic_definitions")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Boost::diagnostic_definitions
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_LIBRARIES_TARGETS}>
                     APPEND)

        if("${boost_Boost_diagnostic_definitions_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Boost::diagnostic_definitions
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         boost_Boost_diagnostic_definitions_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Boost::diagnostic_definitions PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Boost::diagnostic_definitions PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Boost::diagnostic_definitions PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Boost::diagnostic_definitions PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::log_setup APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::log APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::locale APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::iostreams APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::contract APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::unit_test_framework APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::type_erasure APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::timer APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::thread APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::random APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::prg_exec_monitor APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::filesystem APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::coroutine APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::chrono APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::wserialization APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::test_exec_monitor APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::test APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::system APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::stacktrace_noop APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::stacktrace_basic APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::stacktrace_backtrace APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::stacktrace_addr2line APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::stacktrace APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::serialization APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::regex APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::program_options APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::exception APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::date_time APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::context APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::container APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::atomic APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES boost::_libboost APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::boost APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::headers APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::dynamic_linking APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::disable_autolinking APPEND)
    set_property(TARGET boost::boost PROPERTY INTERFACE_LINK_LIBRARIES Boost::diagnostic_definitions APPEND)

########## For the modules (FindXXX)
set(boost_LIBRARIES_RELEASE boost::boost)
