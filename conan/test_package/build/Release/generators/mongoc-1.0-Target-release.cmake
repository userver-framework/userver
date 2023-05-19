# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(mongo-c-driver_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(mongo-c-driver_FRAMEWORKS_FOUND_RELEASE "${mongo-c-driver_FRAMEWORKS_RELEASE}" "${mongo-c-driver_FRAMEWORK_DIRS_RELEASE}")

set(mongo-c-driver_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET mongo-c-driver_DEPS_TARGET)
    add_library(mongo-c-driver_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET mongo-c-driver_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${mongo-c-driver_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${mongo-c-driver_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:openssl::openssl;cyrus-sasl::cyrus-sasl;Snappy::snappy;ZLIB::ZLIB;zstd::libzstd_static;icu::icu;mongo::bson_static>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### mongo-c-driver_DEPS_TARGET to all of them
conan_package_library_targets("${mongo-c-driver_LIBS_RELEASE}"    # libraries
                              "${mongo-c-driver_LIB_DIRS_RELEASE}" # package_libdir
                              mongo-c-driver_DEPS_TARGET
                              mongo-c-driver_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "mongo-c-driver")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${mongo-c-driver_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT mongo::mongoc_static #############

        set(mongo-c-driver_mongo_mongoc_static_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(mongo-c-driver_mongo_mongoc_static_FRAMEWORKS_FOUND_RELEASE "${mongo-c-driver_mongo_mongoc_static_FRAMEWORKS_RELEASE}" "${mongo-c-driver_mongo_mongoc_static_FRAMEWORK_DIRS_RELEASE}")

        set(mongo-c-driver_mongo_mongoc_static_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET mongo-c-driver_mongo_mongoc_static_DEPS_TARGET)
            add_library(mongo-c-driver_mongo_mongoc_static_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET mongo-c-driver_mongo_mongoc_static_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'mongo-c-driver_mongo_mongoc_static_DEPS_TARGET' to all of them
        conan_package_library_targets("${mongo-c-driver_mongo_mongoc_static_LIBS_RELEASE}"
                                      "${mongo-c-driver_mongo_mongoc_static_LIB_DIRS_RELEASE}"
                                      mongo-c-driver_mongo_mongoc_static_DEPS_TARGET
                                      mongo-c-driver_mongo_mongoc_static_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "mongo-c-driver_mongo_mongoc_static")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET mongo::mongoc_static
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_LIBRARIES_TARGETS}>
                     APPEND)

        if("${mongo-c-driver_mongo_mongoc_static_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET mongo::mongoc_static
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         mongo-c-driver_mongo_mongoc_static_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET mongo::mongoc_static PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET mongo::mongoc_static PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET mongo::mongoc_static PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET mongo::mongoc_static PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_mongoc_static_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT mongo::bson_static #############

        set(mongo-c-driver_mongo_bson_static_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(mongo-c-driver_mongo_bson_static_FRAMEWORKS_FOUND_RELEASE "${mongo-c-driver_mongo_bson_static_FRAMEWORKS_RELEASE}" "${mongo-c-driver_mongo_bson_static_FRAMEWORK_DIRS_RELEASE}")

        set(mongo-c-driver_mongo_bson_static_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET mongo-c-driver_mongo_bson_static_DEPS_TARGET)
            add_library(mongo-c-driver_mongo_bson_static_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET mongo-c-driver_mongo_bson_static_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'mongo-c-driver_mongo_bson_static_DEPS_TARGET' to all of them
        conan_package_library_targets("${mongo-c-driver_mongo_bson_static_LIBS_RELEASE}"
                                      "${mongo-c-driver_mongo_bson_static_LIB_DIRS_RELEASE}"
                                      mongo-c-driver_mongo_bson_static_DEPS_TARGET
                                      mongo-c-driver_mongo_bson_static_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "mongo-c-driver_mongo_bson_static")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET mongo::bson_static
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_LIBRARIES_TARGETS}>
                     APPEND)

        if("${mongo-c-driver_mongo_bson_static_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET mongo::bson_static
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         mongo-c-driver_mongo_bson_static_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET mongo::bson_static PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET mongo::bson_static PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET mongo::bson_static PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET mongo::bson_static PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${mongo-c-driver_mongo_bson_static_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET mongo::mongoc_static PROPERTY INTERFACE_LINK_LIBRARIES mongo::mongoc_static APPEND)
    set_property(TARGET mongo::mongoc_static PROPERTY INTERFACE_LINK_LIBRARIES mongo::bson_static APPEND)

########## For the modules (FindXXX)
set(mongo-c-driver_LIBRARIES_RELEASE mongo::mongoc_static)
