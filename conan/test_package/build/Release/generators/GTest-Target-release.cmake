# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(gtest_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(gtest_FRAMEWORKS_FOUND_RELEASE "${gtest_FRAMEWORKS_RELEASE}" "${gtest_FRAMEWORK_DIRS_RELEASE}")

set(gtest_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET gtest_DEPS_TARGET)
    add_library(gtest_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET gtest_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${gtest_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${gtest_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:GTest::gtest;GTest::gtest;GTest::gmock>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### gtest_DEPS_TARGET to all of them
conan_package_library_targets("${gtest_LIBS_RELEASE}"    # libraries
                              "${gtest_LIB_DIRS_RELEASE}" # package_libdir
                              gtest_DEPS_TARGET
                              gtest_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "gtest")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${gtest_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT GTest::gmock_main #############

        set(gtest_GTest_gmock_main_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(gtest_GTest_gmock_main_FRAMEWORKS_FOUND_RELEASE "${gtest_GTest_gmock_main_FRAMEWORKS_RELEASE}" "${gtest_GTest_gmock_main_FRAMEWORK_DIRS_RELEASE}")

        set(gtest_GTest_gmock_main_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET gtest_GTest_gmock_main_DEPS_TARGET)
            add_library(gtest_GTest_gmock_main_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET gtest_GTest_gmock_main_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'gtest_GTest_gmock_main_DEPS_TARGET' to all of them
        conan_package_library_targets("${gtest_GTest_gmock_main_LIBS_RELEASE}"
                                      "${gtest_GTest_gmock_main_LIB_DIRS_RELEASE}"
                                      gtest_GTest_gmock_main_DEPS_TARGET
                                      gtest_GTest_gmock_main_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "gtest_GTest_gmock_main")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET GTest::gmock_main
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_LIBRARIES_TARGETS}>
                     APPEND)

        if("${gtest_GTest_gmock_main_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET GTest::gmock_main
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         gtest_GTest_gmock_main_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET GTest::gmock_main PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET GTest::gmock_main PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET GTest::gmock_main PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET GTest::gmock_main PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_main_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT GTest::gmock #############

        set(gtest_GTest_gmock_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(gtest_GTest_gmock_FRAMEWORKS_FOUND_RELEASE "${gtest_GTest_gmock_FRAMEWORKS_RELEASE}" "${gtest_GTest_gmock_FRAMEWORK_DIRS_RELEASE}")

        set(gtest_GTest_gmock_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET gtest_GTest_gmock_DEPS_TARGET)
            add_library(gtest_GTest_gmock_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET gtest_GTest_gmock_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'gtest_GTest_gmock_DEPS_TARGET' to all of them
        conan_package_library_targets("${gtest_GTest_gmock_LIBS_RELEASE}"
                                      "${gtest_GTest_gmock_LIB_DIRS_RELEASE}"
                                      gtest_GTest_gmock_DEPS_TARGET
                                      gtest_GTest_gmock_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "gtest_GTest_gmock")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET GTest::gmock
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_LIBRARIES_TARGETS}>
                     APPEND)

        if("${gtest_GTest_gmock_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET GTest::gmock
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         gtest_GTest_gmock_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET GTest::gmock PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET GTest::gmock PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET GTest::gmock PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET GTest::gmock PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gmock_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT GTest::gtest_main #############

        set(gtest_GTest_gtest_main_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(gtest_GTest_gtest_main_FRAMEWORKS_FOUND_RELEASE "${gtest_GTest_gtest_main_FRAMEWORKS_RELEASE}" "${gtest_GTest_gtest_main_FRAMEWORK_DIRS_RELEASE}")

        set(gtest_GTest_gtest_main_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET gtest_GTest_gtest_main_DEPS_TARGET)
            add_library(gtest_GTest_gtest_main_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET gtest_GTest_gtest_main_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'gtest_GTest_gtest_main_DEPS_TARGET' to all of them
        conan_package_library_targets("${gtest_GTest_gtest_main_LIBS_RELEASE}"
                                      "${gtest_GTest_gtest_main_LIB_DIRS_RELEASE}"
                                      gtest_GTest_gtest_main_DEPS_TARGET
                                      gtest_GTest_gtest_main_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "gtest_GTest_gtest_main")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET GTest::gtest_main
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_LIBRARIES_TARGETS}>
                     APPEND)

        if("${gtest_GTest_gtest_main_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET GTest::gtest_main
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         gtest_GTest_gtest_main_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET GTest::gtest_main PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET GTest::gtest_main PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET GTest::gtest_main PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET GTest::gtest_main PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_main_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT GTest::gtest #############

        set(gtest_GTest_gtest_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(gtest_GTest_gtest_FRAMEWORKS_FOUND_RELEASE "${gtest_GTest_gtest_FRAMEWORKS_RELEASE}" "${gtest_GTest_gtest_FRAMEWORK_DIRS_RELEASE}")

        set(gtest_GTest_gtest_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET gtest_GTest_gtest_DEPS_TARGET)
            add_library(gtest_GTest_gtest_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET gtest_GTest_gtest_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'gtest_GTest_gtest_DEPS_TARGET' to all of them
        conan_package_library_targets("${gtest_GTest_gtest_LIBS_RELEASE}"
                                      "${gtest_GTest_gtest_LIB_DIRS_RELEASE}"
                                      gtest_GTest_gtest_DEPS_TARGET
                                      gtest_GTest_gtest_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "gtest_GTest_gtest")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET GTest::gtest
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_LIBRARIES_TARGETS}>
                     APPEND)

        if("${gtest_GTest_gtest_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET GTest::gtest
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         gtest_GTest_gtest_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET GTest::gtest PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET GTest::gtest PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET GTest::gtest PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET GTest::gtest PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${gtest_GTest_gtest_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET gtest::gtest PROPERTY INTERFACE_LINK_LIBRARIES GTest::gmock_main APPEND)
    set_property(TARGET gtest::gtest PROPERTY INTERFACE_LINK_LIBRARIES GTest::gmock APPEND)
    set_property(TARGET gtest::gtest PROPERTY INTERFACE_LINK_LIBRARIES GTest::gtest_main APPEND)
    set_property(TARGET gtest::gtest PROPERTY INTERFACE_LINK_LIBRARIES GTest::gtest APPEND)

########## For the modules (FindXXX)
set(gtest_LIBRARIES_RELEASE gtest::gtest)
