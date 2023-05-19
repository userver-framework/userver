# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(icu_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(icu_FRAMEWORKS_FOUND_RELEASE "${icu_FRAMEWORKS_RELEASE}" "${icu_FRAMEWORK_DIRS_RELEASE}")

set(icu_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET icu_DEPS_TARGET)
    add_library(icu_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET icu_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${icu_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${icu_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:ICU::data;ICU::data;ICU::uc;ICU::i18n;ICU::i18n;ICU::uc;ICU::i18n;ICU::uc;ICU::tu;ICU::uc>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### icu_DEPS_TARGET to all of them
conan_package_library_targets("${icu_LIBS_RELEASE}"    # libraries
                              "${icu_LIB_DIRS_RELEASE}" # package_libdir
                              icu_DEPS_TARGET
                              icu_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "icu")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${icu_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT ICU::test #############

        set(icu_ICU_test_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_test_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_test_FRAMEWORKS_RELEASE}" "${icu_ICU_test_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_test_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_test_DEPS_TARGET)
            add_library(icu_ICU_test_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_test_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_test_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_test_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_test_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_test_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_test_LIBS_RELEASE}"
                                      "${icu_ICU_test_LIB_DIRS_RELEASE}"
                                      icu_ICU_test_DEPS_TARGET
                                      icu_ICU_test_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_test")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::test
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_test_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_test_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_test_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::test
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_test_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::test PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_test_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::test PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_test_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::test PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_test_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::test PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_test_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::tu #############

        set(icu_ICU_tu_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_tu_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_tu_FRAMEWORKS_RELEASE}" "${icu_ICU_tu_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_tu_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_tu_DEPS_TARGET)
            add_library(icu_ICU_tu_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_tu_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_tu_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_tu_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_tu_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_tu_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_tu_LIBS_RELEASE}"
                                      "${icu_ICU_tu_LIB_DIRS_RELEASE}"
                                      icu_ICU_tu_DEPS_TARGET
                                      icu_ICU_tu_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_tu")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::tu
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_tu_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_tu_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_tu_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::tu
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_tu_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::tu PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_tu_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::tu PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_tu_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::tu PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_tu_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::tu PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_tu_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::io #############

        set(icu_ICU_io_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_io_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_io_FRAMEWORKS_RELEASE}" "${icu_ICU_io_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_io_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_io_DEPS_TARGET)
            add_library(icu_ICU_io_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_io_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_io_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_io_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_io_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_io_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_io_LIBS_RELEASE}"
                                      "${icu_ICU_io_LIB_DIRS_RELEASE}"
                                      icu_ICU_io_DEPS_TARGET
                                      icu_ICU_io_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_io")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::io
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_io_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_io_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_io_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::io
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_io_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::io PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_io_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::io PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_io_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::io PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_io_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::io PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_io_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::in #############

        set(icu_ICU_in_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_in_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_in_FRAMEWORKS_RELEASE}" "${icu_ICU_in_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_in_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_in_DEPS_TARGET)
            add_library(icu_ICU_in_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_in_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_in_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_in_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_in_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_in_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_in_LIBS_RELEASE}"
                                      "${icu_ICU_in_LIB_DIRS_RELEASE}"
                                      icu_ICU_in_DEPS_TARGET
                                      icu_ICU_in_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_in")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::in
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_in_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_in_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_in_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::in
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_in_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::in PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_in_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::in PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_in_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::in PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_in_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::in PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_in_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::i18n #############

        set(icu_ICU_i18n_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_i18n_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_i18n_FRAMEWORKS_RELEASE}" "${icu_ICU_i18n_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_i18n_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_i18n_DEPS_TARGET)
            add_library(icu_ICU_i18n_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_i18n_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_i18n_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_i18n_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_i18n_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_i18n_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_i18n_LIBS_RELEASE}"
                                      "${icu_ICU_i18n_LIB_DIRS_RELEASE}"
                                      icu_ICU_i18n_DEPS_TARGET
                                      icu_ICU_i18n_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_i18n")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::i18n
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_i18n_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_i18n_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_i18n_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::i18n
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_i18n_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::i18n PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_i18n_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::i18n PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_i18n_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::i18n PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_i18n_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::i18n PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_i18n_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::uc #############

        set(icu_ICU_uc_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_uc_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_uc_FRAMEWORKS_RELEASE}" "${icu_ICU_uc_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_uc_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_uc_DEPS_TARGET)
            add_library(icu_ICU_uc_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_uc_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_uc_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_uc_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_uc_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_uc_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_uc_LIBS_RELEASE}"
                                      "${icu_ICU_uc_LIB_DIRS_RELEASE}"
                                      icu_ICU_uc_DEPS_TARGET
                                      icu_ICU_uc_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_uc")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::uc
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_uc_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_uc_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_uc_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::uc
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_uc_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::uc PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_uc_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::uc PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_uc_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::uc PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_uc_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::uc PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_uc_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::dt #############

        set(icu_ICU_dt_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_dt_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_dt_FRAMEWORKS_RELEASE}" "${icu_ICU_dt_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_dt_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_dt_DEPS_TARGET)
            add_library(icu_ICU_dt_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_dt_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_dt_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_dt_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_dt_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_dt_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_dt_LIBS_RELEASE}"
                                      "${icu_ICU_dt_LIB_DIRS_RELEASE}"
                                      icu_ICU_dt_DEPS_TARGET
                                      icu_ICU_dt_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_dt")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::dt
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_dt_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_dt_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_dt_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::dt
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_dt_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::dt PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_dt_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::dt PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_dt_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::dt PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_dt_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::dt PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_dt_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT ICU::data #############

        set(icu_ICU_data_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(icu_ICU_data_FRAMEWORKS_FOUND_RELEASE "${icu_ICU_data_FRAMEWORKS_RELEASE}" "${icu_ICU_data_FRAMEWORK_DIRS_RELEASE}")

        set(icu_ICU_data_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET icu_ICU_data_DEPS_TARGET)
            add_library(icu_ICU_data_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET icu_ICU_data_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_data_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_data_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_data_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'icu_ICU_data_DEPS_TARGET' to all of them
        conan_package_library_targets("${icu_ICU_data_LIBS_RELEASE}"
                                      "${icu_ICU_data_LIB_DIRS_RELEASE}"
                                      icu_ICU_data_DEPS_TARGET
                                      icu_ICU_data_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "icu_ICU_data")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET ICU::data
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${icu_ICU_data_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${icu_ICU_data_LIBRARIES_TARGETS}>
                     APPEND)

        if("${icu_ICU_data_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET ICU::data
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         icu_ICU_data_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET ICU::data PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_data_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET ICU::data PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${icu_ICU_data_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET ICU::data PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${icu_ICU_data_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET ICU::data PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${icu_ICU_data_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::test APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::tu APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::io APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::in APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::i18n APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::uc APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::dt APPEND)
    set_property(TARGET icu::icu PROPERTY INTERFACE_LINK_LIBRARIES ICU::data APPEND)

########## For the modules (FindXXX)
set(icu_LIBRARIES_RELEASE icu::icu)
