########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND boost_COMPONENT_NAMES Boost::diagnostic_definitions Boost::disable_autolinking Boost::dynamic_linking Boost::headers Boost::boost boost::_libboost Boost::atomic Boost::container Boost::context Boost::date_time Boost::exception Boost::program_options Boost::regex Boost::serialization Boost::stacktrace Boost::stacktrace_addr2line Boost::stacktrace_backtrace Boost::stacktrace_basic Boost::stacktrace_noop Boost::system Boost::test Boost::test_exec_monitor Boost::wserialization Boost::chrono Boost::coroutine Boost::filesystem Boost::prg_exec_monitor Boost::random Boost::thread Boost::timer Boost::type_erasure Boost::unit_test_framework Boost::contract Boost::iostreams Boost::locale Boost::log Boost::log_setup)
list(REMOVE_DUPLICATES boost_COMPONENT_NAMES)
list(APPEND boost_FIND_DEPENDENCY_NAMES BZip2 ZLIB Iconv libbacktrace)
list(REMOVE_DUPLICATES boost_FIND_DEPENDENCY_NAMES)
set(ZLIB_FIND_MODE "NO_MODULE")
set(BZip2_FIND_MODE "NO_MODULE")
set(libbacktrace_FIND_MODE "NO_MODULE")
set(Iconv_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(boost_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/boost/1.79.0/_/_/package/effc61087c57d5292f5fa96377e7b5e2c60734c9")
set(boost_BUILD_MODULES_PATHS_RELEASE )


set(boost_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_RES_DIRS_RELEASE )
set(boost_DEFINITIONS_RELEASE "-DBOOST_STACKTRACE_USE_NOOP"
			"-DBOOST_STACKTRACE_USE_BACKTRACE"
			"-DBOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"-DBOOST_STACKTRACE_USE_ADDR2LINE"
			"-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED")
set(boost_SHARED_LINK_FLAGS_RELEASE )
set(boost_EXE_LINK_FLAGS_RELEASE )
set(boost_OBJECTS_RELEASE )
set(boost_COMPILE_DEFINITIONS_RELEASE "BOOST_STACKTRACE_USE_NOOP"
			"BOOST_STACKTRACE_USE_BACKTRACE"
			"BOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"BOOST_STACKTRACE_USE_ADDR2LINE"
			"BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED")
set(boost_COMPILE_OPTIONS_C_RELEASE )
set(boost_COMPILE_OPTIONS_CXX_RELEASE )
set(boost_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_LIBS_RELEASE boost_log_setup boost_log boost_locale boost_iostreams boost_contract boost_unit_test_framework boost_type_erasure boost_timer boost_thread boost_random boost_prg_exec_monitor boost_filesystem boost_coroutine boost_chrono boost_wserialization boost_test_exec_monitor boost_stacktrace_noop boost_stacktrace_basic boost_stacktrace_backtrace boost_stacktrace_addr2line boost_serialization boost_regex boost_program_options boost_exception boost_date_time boost_context boost_container boost_atomic)
set(boost_SYSTEM_LIBS_RELEASE )
set(boost_FRAMEWORK_DIRS_RELEASE )
set(boost_FRAMEWORKS_RELEASE )
set(boost_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(boost_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_COMPILE_OPTIONS_C_RELEASE}>")
set(boost_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_EXE_LINK_FLAGS_RELEASE}>")


set(boost_COMPONENTS_RELEASE Boost::diagnostic_definitions Boost::disable_autolinking Boost::dynamic_linking Boost::headers Boost::boost boost::_libboost Boost::atomic Boost::container Boost::context Boost::date_time Boost::exception Boost::program_options Boost::regex Boost::serialization Boost::stacktrace Boost::stacktrace_addr2line Boost::stacktrace_backtrace Boost::stacktrace_basic Boost::stacktrace_noop Boost::system Boost::test Boost::test_exec_monitor Boost::wserialization Boost::chrono Boost::coroutine Boost::filesystem Boost::prg_exec_monitor Boost::random Boost::thread Boost::timer Boost::type_erasure Boost::unit_test_framework Boost::contract Boost::iostreams Boost::locale Boost::log Boost::log_setup)
########### COMPONENT Boost::log_setup VARIABLES ############################################

set(boost_Boost_log_setup_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_log_setup_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_log_setup_RES_DIRS_RELEASE )
set(boost_Boost_log_setup_DEFINITIONS_RELEASE )
set(boost_Boost_log_setup_OBJECTS_RELEASE )
set(boost_Boost_log_setup_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_log_setup_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_log_setup_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_log_setup_LIBS_RELEASE boost_log_setup)
set(boost_Boost_log_setup_SYSTEM_LIBS_RELEASE )
set(boost_Boost_log_setup_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_log_setup_FRAMEWORKS_RELEASE )
set(boost_Boost_log_setup_DEPENDENCIES_RELEASE Boost::log boost::_libboost)
set(boost_Boost_log_setup_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_log_setup_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_log_setup_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_log_setup_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_log_setup_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_log_setup_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_log_setup_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_log_setup_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_log_setup_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::log VARIABLES ############################################

set(boost_Boost_log_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_log_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_log_RES_DIRS_RELEASE )
set(boost_Boost_log_DEFINITIONS_RELEASE )
set(boost_Boost_log_OBJECTS_RELEASE )
set(boost_Boost_log_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_log_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_log_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_log_LIBS_RELEASE boost_log)
set(boost_Boost_log_SYSTEM_LIBS_RELEASE )
set(boost_Boost_log_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_log_FRAMEWORKS_RELEASE )
set(boost_Boost_log_DEPENDENCIES_RELEASE Boost::atomic Boost::container Boost::date_time Boost::exception Boost::filesystem Boost::random Boost::regex Boost::system Boost::thread boost::_libboost)
set(boost_Boost_log_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_log_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_log_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_log_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_log_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_log_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_log_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_log_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_log_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::locale VARIABLES ############################################

set(boost_Boost_locale_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_locale_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_locale_RES_DIRS_RELEASE )
set(boost_Boost_locale_DEFINITIONS_RELEASE )
set(boost_Boost_locale_OBJECTS_RELEASE )
set(boost_Boost_locale_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_locale_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_locale_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_locale_LIBS_RELEASE boost_locale)
set(boost_Boost_locale_SYSTEM_LIBS_RELEASE )
set(boost_Boost_locale_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_locale_FRAMEWORKS_RELEASE )
set(boost_Boost_locale_DEPENDENCIES_RELEASE Boost::thread boost::_libboost Iconv::Iconv)
set(boost_Boost_locale_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_locale_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_locale_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_locale_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_locale_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_locale_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_locale_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_locale_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_locale_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::iostreams VARIABLES ############################################

set(boost_Boost_iostreams_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_iostreams_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_iostreams_RES_DIRS_RELEASE )
set(boost_Boost_iostreams_DEFINITIONS_RELEASE )
set(boost_Boost_iostreams_OBJECTS_RELEASE )
set(boost_Boost_iostreams_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_iostreams_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_iostreams_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_iostreams_LIBS_RELEASE boost_iostreams)
set(boost_Boost_iostreams_SYSTEM_LIBS_RELEASE )
set(boost_Boost_iostreams_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_iostreams_FRAMEWORKS_RELEASE )
set(boost_Boost_iostreams_DEPENDENCIES_RELEASE Boost::random Boost::regex boost::_libboost BZip2::BZip2 ZLIB::ZLIB)
set(boost_Boost_iostreams_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_iostreams_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_iostreams_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_iostreams_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_iostreams_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_iostreams_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_iostreams_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_iostreams_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_iostreams_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::contract VARIABLES ############################################

set(boost_Boost_contract_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_contract_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_contract_RES_DIRS_RELEASE )
set(boost_Boost_contract_DEFINITIONS_RELEASE )
set(boost_Boost_contract_OBJECTS_RELEASE )
set(boost_Boost_contract_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_contract_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_contract_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_contract_LIBS_RELEASE boost_contract)
set(boost_Boost_contract_SYSTEM_LIBS_RELEASE )
set(boost_Boost_contract_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_contract_FRAMEWORKS_RELEASE )
set(boost_Boost_contract_DEPENDENCIES_RELEASE Boost::exception Boost::thread boost::_libboost)
set(boost_Boost_contract_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_contract_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_contract_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_contract_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_contract_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_contract_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_contract_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_contract_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_contract_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::unit_test_framework VARIABLES ############################################

set(boost_Boost_unit_test_framework_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_unit_test_framework_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_unit_test_framework_RES_DIRS_RELEASE )
set(boost_Boost_unit_test_framework_DEFINITIONS_RELEASE )
set(boost_Boost_unit_test_framework_OBJECTS_RELEASE )
set(boost_Boost_unit_test_framework_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_unit_test_framework_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_unit_test_framework_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_unit_test_framework_LIBS_RELEASE boost_unit_test_framework)
set(boost_Boost_unit_test_framework_SYSTEM_LIBS_RELEASE )
set(boost_Boost_unit_test_framework_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_unit_test_framework_FRAMEWORKS_RELEASE )
set(boost_Boost_unit_test_framework_DEPENDENCIES_RELEASE Boost::prg_exec_monitor Boost::test Boost::test_exec_monitor boost::_libboost)
set(boost_Boost_unit_test_framework_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_unit_test_framework_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_unit_test_framework_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_unit_test_framework_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_unit_test_framework_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_unit_test_framework_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_unit_test_framework_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_unit_test_framework_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_unit_test_framework_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::type_erasure VARIABLES ############################################

set(boost_Boost_type_erasure_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_type_erasure_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_type_erasure_RES_DIRS_RELEASE )
set(boost_Boost_type_erasure_DEFINITIONS_RELEASE )
set(boost_Boost_type_erasure_OBJECTS_RELEASE )
set(boost_Boost_type_erasure_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_type_erasure_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_type_erasure_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_type_erasure_LIBS_RELEASE boost_type_erasure)
set(boost_Boost_type_erasure_SYSTEM_LIBS_RELEASE )
set(boost_Boost_type_erasure_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_type_erasure_FRAMEWORKS_RELEASE )
set(boost_Boost_type_erasure_DEPENDENCIES_RELEASE Boost::thread boost::_libboost)
set(boost_Boost_type_erasure_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_type_erasure_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_type_erasure_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_type_erasure_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_type_erasure_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_type_erasure_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_type_erasure_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_type_erasure_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_type_erasure_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::timer VARIABLES ############################################

set(boost_Boost_timer_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_timer_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_timer_RES_DIRS_RELEASE )
set(boost_Boost_timer_DEFINITIONS_RELEASE )
set(boost_Boost_timer_OBJECTS_RELEASE )
set(boost_Boost_timer_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_timer_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_timer_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_timer_LIBS_RELEASE boost_timer)
set(boost_Boost_timer_SYSTEM_LIBS_RELEASE )
set(boost_Boost_timer_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_timer_FRAMEWORKS_RELEASE )
set(boost_Boost_timer_DEPENDENCIES_RELEASE Boost::chrono Boost::system boost::_libboost)
set(boost_Boost_timer_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_timer_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_timer_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_timer_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_timer_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_timer_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_timer_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_timer_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_timer_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::thread VARIABLES ############################################

set(boost_Boost_thread_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_thread_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_thread_RES_DIRS_RELEASE )
set(boost_Boost_thread_DEFINITIONS_RELEASE )
set(boost_Boost_thread_OBJECTS_RELEASE )
set(boost_Boost_thread_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_thread_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_thread_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_thread_LIBS_RELEASE boost_thread)
set(boost_Boost_thread_SYSTEM_LIBS_RELEASE )
set(boost_Boost_thread_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_thread_FRAMEWORKS_RELEASE )
set(boost_Boost_thread_DEPENDENCIES_RELEASE Boost::atomic Boost::chrono Boost::container Boost::date_time Boost::exception Boost::system boost::_libboost)
set(boost_Boost_thread_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_thread_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_thread_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_thread_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_thread_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_thread_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_thread_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_thread_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_thread_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::random VARIABLES ############################################

set(boost_Boost_random_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_random_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_random_RES_DIRS_RELEASE )
set(boost_Boost_random_DEFINITIONS_RELEASE )
set(boost_Boost_random_OBJECTS_RELEASE )
set(boost_Boost_random_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_random_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_random_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_random_LIBS_RELEASE boost_random)
set(boost_Boost_random_SYSTEM_LIBS_RELEASE )
set(boost_Boost_random_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_random_FRAMEWORKS_RELEASE )
set(boost_Boost_random_DEPENDENCIES_RELEASE Boost::system boost::_libboost)
set(boost_Boost_random_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_random_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_random_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_random_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_random_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_random_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_random_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_random_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_random_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::prg_exec_monitor VARIABLES ############################################

set(boost_Boost_prg_exec_monitor_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_prg_exec_monitor_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_prg_exec_monitor_RES_DIRS_RELEASE )
set(boost_Boost_prg_exec_monitor_DEFINITIONS_RELEASE )
set(boost_Boost_prg_exec_monitor_OBJECTS_RELEASE )
set(boost_Boost_prg_exec_monitor_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_prg_exec_monitor_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_prg_exec_monitor_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_prg_exec_monitor_LIBS_RELEASE boost_prg_exec_monitor)
set(boost_Boost_prg_exec_monitor_SYSTEM_LIBS_RELEASE )
set(boost_Boost_prg_exec_monitor_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_prg_exec_monitor_FRAMEWORKS_RELEASE )
set(boost_Boost_prg_exec_monitor_DEPENDENCIES_RELEASE Boost::test boost::_libboost)
set(boost_Boost_prg_exec_monitor_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_prg_exec_monitor_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_prg_exec_monitor_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_prg_exec_monitor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_prg_exec_monitor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_prg_exec_monitor_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_prg_exec_monitor_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_prg_exec_monitor_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_prg_exec_monitor_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::filesystem VARIABLES ############################################

set(boost_Boost_filesystem_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_filesystem_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_filesystem_RES_DIRS_RELEASE )
set(boost_Boost_filesystem_DEFINITIONS_RELEASE )
set(boost_Boost_filesystem_OBJECTS_RELEASE )
set(boost_Boost_filesystem_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_filesystem_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_filesystem_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_filesystem_LIBS_RELEASE boost_filesystem)
set(boost_Boost_filesystem_SYSTEM_LIBS_RELEASE )
set(boost_Boost_filesystem_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_filesystem_FRAMEWORKS_RELEASE )
set(boost_Boost_filesystem_DEPENDENCIES_RELEASE Boost::atomic Boost::system boost::_libboost)
set(boost_Boost_filesystem_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_filesystem_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_filesystem_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_filesystem_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_filesystem_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_filesystem_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_filesystem_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_filesystem_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_filesystem_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::coroutine VARIABLES ############################################

set(boost_Boost_coroutine_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_coroutine_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_coroutine_RES_DIRS_RELEASE )
set(boost_Boost_coroutine_DEFINITIONS_RELEASE )
set(boost_Boost_coroutine_OBJECTS_RELEASE )
set(boost_Boost_coroutine_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_coroutine_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_coroutine_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_coroutine_LIBS_RELEASE boost_coroutine)
set(boost_Boost_coroutine_SYSTEM_LIBS_RELEASE )
set(boost_Boost_coroutine_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_coroutine_FRAMEWORKS_RELEASE )
set(boost_Boost_coroutine_DEPENDENCIES_RELEASE Boost::context Boost::exception Boost::system boost::_libboost)
set(boost_Boost_coroutine_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_coroutine_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_coroutine_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_coroutine_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_coroutine_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_coroutine_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_coroutine_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_coroutine_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_coroutine_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::chrono VARIABLES ############################################

set(boost_Boost_chrono_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_chrono_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_chrono_RES_DIRS_RELEASE )
set(boost_Boost_chrono_DEFINITIONS_RELEASE )
set(boost_Boost_chrono_OBJECTS_RELEASE )
set(boost_Boost_chrono_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_chrono_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_chrono_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_chrono_LIBS_RELEASE boost_chrono)
set(boost_Boost_chrono_SYSTEM_LIBS_RELEASE )
set(boost_Boost_chrono_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_chrono_FRAMEWORKS_RELEASE )
set(boost_Boost_chrono_DEPENDENCIES_RELEASE Boost::system boost::_libboost)
set(boost_Boost_chrono_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_chrono_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_chrono_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_chrono_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_chrono_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_chrono_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_chrono_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_chrono_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_chrono_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::wserialization VARIABLES ############################################

set(boost_Boost_wserialization_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_wserialization_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_wserialization_RES_DIRS_RELEASE )
set(boost_Boost_wserialization_DEFINITIONS_RELEASE )
set(boost_Boost_wserialization_OBJECTS_RELEASE )
set(boost_Boost_wserialization_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_wserialization_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_wserialization_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_wserialization_LIBS_RELEASE boost_wserialization)
set(boost_Boost_wserialization_SYSTEM_LIBS_RELEASE )
set(boost_Boost_wserialization_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_wserialization_FRAMEWORKS_RELEASE )
set(boost_Boost_wserialization_DEPENDENCIES_RELEASE Boost::serialization boost::_libboost)
set(boost_Boost_wserialization_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_wserialization_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_wserialization_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_wserialization_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_wserialization_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_wserialization_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_wserialization_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_wserialization_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_wserialization_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::test_exec_monitor VARIABLES ############################################

set(boost_Boost_test_exec_monitor_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_test_exec_monitor_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_test_exec_monitor_RES_DIRS_RELEASE )
set(boost_Boost_test_exec_monitor_DEFINITIONS_RELEASE )
set(boost_Boost_test_exec_monitor_OBJECTS_RELEASE )
set(boost_Boost_test_exec_monitor_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_test_exec_monitor_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_test_exec_monitor_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_test_exec_monitor_LIBS_RELEASE boost_test_exec_monitor)
set(boost_Boost_test_exec_monitor_SYSTEM_LIBS_RELEASE )
set(boost_Boost_test_exec_monitor_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_test_exec_monitor_FRAMEWORKS_RELEASE )
set(boost_Boost_test_exec_monitor_DEPENDENCIES_RELEASE Boost::test boost::_libboost)
set(boost_Boost_test_exec_monitor_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_test_exec_monitor_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_test_exec_monitor_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_test_exec_monitor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_test_exec_monitor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_test_exec_monitor_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_test_exec_monitor_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_test_exec_monitor_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_test_exec_monitor_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::test VARIABLES ############################################

set(boost_Boost_test_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_test_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_test_RES_DIRS_RELEASE )
set(boost_Boost_test_DEFINITIONS_RELEASE )
set(boost_Boost_test_OBJECTS_RELEASE )
set(boost_Boost_test_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_test_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_test_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_test_LIBS_RELEASE )
set(boost_Boost_test_SYSTEM_LIBS_RELEASE )
set(boost_Boost_test_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_test_FRAMEWORKS_RELEASE )
set(boost_Boost_test_DEPENDENCIES_RELEASE Boost::exception boost::_libboost)
set(boost_Boost_test_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_test_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_test_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_test_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_test_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_test_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_test_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_test_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_test_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::system VARIABLES ############################################

set(boost_Boost_system_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_system_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_system_RES_DIRS_RELEASE )
set(boost_Boost_system_DEFINITIONS_RELEASE )
set(boost_Boost_system_OBJECTS_RELEASE )
set(boost_Boost_system_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_system_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_system_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_system_LIBS_RELEASE )
set(boost_Boost_system_SYSTEM_LIBS_RELEASE )
set(boost_Boost_system_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_system_FRAMEWORKS_RELEASE )
set(boost_Boost_system_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_system_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_system_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_system_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_system_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_system_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_system_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_system_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_system_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_system_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::stacktrace_noop VARIABLES ############################################

set(boost_Boost_stacktrace_noop_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_stacktrace_noop_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_stacktrace_noop_RES_DIRS_RELEASE )
set(boost_Boost_stacktrace_noop_DEFINITIONS_RELEASE "-DBOOST_STACKTRACE_USE_NOOP")
set(boost_Boost_stacktrace_noop_OBJECTS_RELEASE )
set(boost_Boost_stacktrace_noop_COMPILE_DEFINITIONS_RELEASE "BOOST_STACKTRACE_USE_NOOP")
set(boost_Boost_stacktrace_noop_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_stacktrace_noop_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_stacktrace_noop_LIBS_RELEASE boost_stacktrace_noop)
set(boost_Boost_stacktrace_noop_SYSTEM_LIBS_RELEASE )
set(boost_Boost_stacktrace_noop_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_stacktrace_noop_FRAMEWORKS_RELEASE )
set(boost_Boost_stacktrace_noop_DEPENDENCIES_RELEASE Boost::stacktrace boost::_libboost)
set(boost_Boost_stacktrace_noop_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_stacktrace_noop_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_stacktrace_noop_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_stacktrace_noop_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_stacktrace_noop_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_stacktrace_noop_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_stacktrace_noop_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_stacktrace_noop_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_stacktrace_noop_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::stacktrace_basic VARIABLES ############################################

set(boost_Boost_stacktrace_basic_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_stacktrace_basic_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_stacktrace_basic_RES_DIRS_RELEASE )
set(boost_Boost_stacktrace_basic_DEFINITIONS_RELEASE )
set(boost_Boost_stacktrace_basic_OBJECTS_RELEASE )
set(boost_Boost_stacktrace_basic_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_stacktrace_basic_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_stacktrace_basic_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_stacktrace_basic_LIBS_RELEASE boost_stacktrace_basic)
set(boost_Boost_stacktrace_basic_SYSTEM_LIBS_RELEASE )
set(boost_Boost_stacktrace_basic_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_stacktrace_basic_FRAMEWORKS_RELEASE )
set(boost_Boost_stacktrace_basic_DEPENDENCIES_RELEASE Boost::stacktrace boost::_libboost)
set(boost_Boost_stacktrace_basic_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_stacktrace_basic_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_stacktrace_basic_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_stacktrace_basic_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_stacktrace_basic_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_stacktrace_basic_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_stacktrace_basic_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_stacktrace_basic_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_stacktrace_basic_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::stacktrace_backtrace VARIABLES ############################################

set(boost_Boost_stacktrace_backtrace_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_stacktrace_backtrace_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_stacktrace_backtrace_RES_DIRS_RELEASE )
set(boost_Boost_stacktrace_backtrace_DEFINITIONS_RELEASE "-DBOOST_STACKTRACE_USE_BACKTRACE")
set(boost_Boost_stacktrace_backtrace_OBJECTS_RELEASE )
set(boost_Boost_stacktrace_backtrace_COMPILE_DEFINITIONS_RELEASE "BOOST_STACKTRACE_USE_BACKTRACE")
set(boost_Boost_stacktrace_backtrace_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_stacktrace_backtrace_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_stacktrace_backtrace_LIBS_RELEASE boost_stacktrace_backtrace)
set(boost_Boost_stacktrace_backtrace_SYSTEM_LIBS_RELEASE )
set(boost_Boost_stacktrace_backtrace_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_stacktrace_backtrace_FRAMEWORKS_RELEASE )
set(boost_Boost_stacktrace_backtrace_DEPENDENCIES_RELEASE Boost::stacktrace boost::_libboost libbacktrace::libbacktrace)
set(boost_Boost_stacktrace_backtrace_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_stacktrace_backtrace_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_stacktrace_backtrace_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_stacktrace_backtrace_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_stacktrace_backtrace_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_stacktrace_backtrace_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_stacktrace_backtrace_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_stacktrace_backtrace_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_stacktrace_backtrace_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::stacktrace_addr2line VARIABLES ############################################

set(boost_Boost_stacktrace_addr2line_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_stacktrace_addr2line_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_stacktrace_addr2line_RES_DIRS_RELEASE )
set(boost_Boost_stacktrace_addr2line_DEFINITIONS_RELEASE "-DBOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"-DBOOST_STACKTRACE_USE_ADDR2LINE")
set(boost_Boost_stacktrace_addr2line_OBJECTS_RELEASE )
set(boost_Boost_stacktrace_addr2line_COMPILE_DEFINITIONS_RELEASE "BOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"BOOST_STACKTRACE_USE_ADDR2LINE")
set(boost_Boost_stacktrace_addr2line_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_stacktrace_addr2line_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_stacktrace_addr2line_LIBS_RELEASE boost_stacktrace_addr2line)
set(boost_Boost_stacktrace_addr2line_SYSTEM_LIBS_RELEASE )
set(boost_Boost_stacktrace_addr2line_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_stacktrace_addr2line_FRAMEWORKS_RELEASE )
set(boost_Boost_stacktrace_addr2line_DEPENDENCIES_RELEASE Boost::stacktrace boost::_libboost)
set(boost_Boost_stacktrace_addr2line_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_stacktrace_addr2line_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_stacktrace_addr2line_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_stacktrace_addr2line_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_stacktrace_addr2line_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_stacktrace_addr2line_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_stacktrace_addr2line_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_stacktrace_addr2line_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_stacktrace_addr2line_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::stacktrace VARIABLES ############################################

set(boost_Boost_stacktrace_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_stacktrace_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_stacktrace_RES_DIRS_RELEASE )
set(boost_Boost_stacktrace_DEFINITIONS_RELEASE "-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED")
set(boost_Boost_stacktrace_OBJECTS_RELEASE )
set(boost_Boost_stacktrace_COMPILE_DEFINITIONS_RELEASE "BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED")
set(boost_Boost_stacktrace_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_stacktrace_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_stacktrace_LIBS_RELEASE )
set(boost_Boost_stacktrace_SYSTEM_LIBS_RELEASE )
set(boost_Boost_stacktrace_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_stacktrace_FRAMEWORKS_RELEASE )
set(boost_Boost_stacktrace_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_stacktrace_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_stacktrace_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_stacktrace_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_stacktrace_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_stacktrace_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_stacktrace_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_stacktrace_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_stacktrace_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_stacktrace_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::serialization VARIABLES ############################################

set(boost_Boost_serialization_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_serialization_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_serialization_RES_DIRS_RELEASE )
set(boost_Boost_serialization_DEFINITIONS_RELEASE )
set(boost_Boost_serialization_OBJECTS_RELEASE )
set(boost_Boost_serialization_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_serialization_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_serialization_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_serialization_LIBS_RELEASE boost_serialization)
set(boost_Boost_serialization_SYSTEM_LIBS_RELEASE )
set(boost_Boost_serialization_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_serialization_FRAMEWORKS_RELEASE )
set(boost_Boost_serialization_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_serialization_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_serialization_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_serialization_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_serialization_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_serialization_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_serialization_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_serialization_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_serialization_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_serialization_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::regex VARIABLES ############################################

set(boost_Boost_regex_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_regex_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_regex_RES_DIRS_RELEASE )
set(boost_Boost_regex_DEFINITIONS_RELEASE )
set(boost_Boost_regex_OBJECTS_RELEASE )
set(boost_Boost_regex_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_regex_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_regex_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_regex_LIBS_RELEASE boost_regex)
set(boost_Boost_regex_SYSTEM_LIBS_RELEASE )
set(boost_Boost_regex_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_regex_FRAMEWORKS_RELEASE )
set(boost_Boost_regex_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_regex_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_regex_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_regex_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_regex_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_regex_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_regex_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_regex_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_regex_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_regex_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::program_options VARIABLES ############################################

set(boost_Boost_program_options_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_program_options_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_program_options_RES_DIRS_RELEASE )
set(boost_Boost_program_options_DEFINITIONS_RELEASE )
set(boost_Boost_program_options_OBJECTS_RELEASE )
set(boost_Boost_program_options_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_program_options_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_program_options_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_program_options_LIBS_RELEASE boost_program_options)
set(boost_Boost_program_options_SYSTEM_LIBS_RELEASE )
set(boost_Boost_program_options_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_program_options_FRAMEWORKS_RELEASE )
set(boost_Boost_program_options_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_program_options_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_program_options_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_program_options_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_program_options_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_program_options_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_program_options_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_program_options_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_program_options_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_program_options_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::exception VARIABLES ############################################

set(boost_Boost_exception_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_exception_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_exception_RES_DIRS_RELEASE )
set(boost_Boost_exception_DEFINITIONS_RELEASE )
set(boost_Boost_exception_OBJECTS_RELEASE )
set(boost_Boost_exception_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_exception_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_exception_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_exception_LIBS_RELEASE boost_exception)
set(boost_Boost_exception_SYSTEM_LIBS_RELEASE )
set(boost_Boost_exception_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_exception_FRAMEWORKS_RELEASE )
set(boost_Boost_exception_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_exception_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_exception_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_exception_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_exception_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_exception_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_exception_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_exception_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_exception_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_exception_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::date_time VARIABLES ############################################

set(boost_Boost_date_time_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_date_time_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_date_time_RES_DIRS_RELEASE )
set(boost_Boost_date_time_DEFINITIONS_RELEASE )
set(boost_Boost_date_time_OBJECTS_RELEASE )
set(boost_Boost_date_time_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_date_time_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_date_time_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_date_time_LIBS_RELEASE boost_date_time)
set(boost_Boost_date_time_SYSTEM_LIBS_RELEASE )
set(boost_Boost_date_time_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_date_time_FRAMEWORKS_RELEASE )
set(boost_Boost_date_time_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_date_time_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_date_time_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_date_time_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_date_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_date_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_date_time_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_date_time_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_date_time_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_date_time_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::context VARIABLES ############################################

set(boost_Boost_context_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_context_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_context_RES_DIRS_RELEASE )
set(boost_Boost_context_DEFINITIONS_RELEASE )
set(boost_Boost_context_OBJECTS_RELEASE )
set(boost_Boost_context_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_context_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_context_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_context_LIBS_RELEASE boost_context)
set(boost_Boost_context_SYSTEM_LIBS_RELEASE )
set(boost_Boost_context_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_context_FRAMEWORKS_RELEASE )
set(boost_Boost_context_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_context_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_context_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_context_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_context_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_context_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_context_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_context_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_context_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_context_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::container VARIABLES ############################################

set(boost_Boost_container_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_container_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_container_RES_DIRS_RELEASE )
set(boost_Boost_container_DEFINITIONS_RELEASE )
set(boost_Boost_container_OBJECTS_RELEASE )
set(boost_Boost_container_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_container_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_container_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_container_LIBS_RELEASE boost_container)
set(boost_Boost_container_SYSTEM_LIBS_RELEASE )
set(boost_Boost_container_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_container_FRAMEWORKS_RELEASE )
set(boost_Boost_container_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_container_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_container_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_container_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_container_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_container_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_container_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_container_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_container_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_container_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::atomic VARIABLES ############################################

set(boost_Boost_atomic_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_atomic_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_atomic_RES_DIRS_RELEASE )
set(boost_Boost_atomic_DEFINITIONS_RELEASE )
set(boost_Boost_atomic_OBJECTS_RELEASE )
set(boost_Boost_atomic_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_atomic_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_atomic_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_atomic_LIBS_RELEASE boost_atomic)
set(boost_Boost_atomic_SYSTEM_LIBS_RELEASE )
set(boost_Boost_atomic_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_atomic_FRAMEWORKS_RELEASE )
set(boost_Boost_atomic_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_atomic_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_atomic_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_atomic_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_atomic_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_atomic_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_atomic_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_atomic_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_atomic_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_atomic_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT boost::_libboost VARIABLES ############################################

set(boost_boost__libboost_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_boost__libboost_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_boost__libboost_RES_DIRS_RELEASE )
set(boost_boost__libboost_DEFINITIONS_RELEASE )
set(boost_boost__libboost_OBJECTS_RELEASE )
set(boost_boost__libboost_COMPILE_DEFINITIONS_RELEASE )
set(boost_boost__libboost_COMPILE_OPTIONS_C_RELEASE "")
set(boost_boost__libboost_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_boost__libboost_LIBS_RELEASE )
set(boost_boost__libboost_SYSTEM_LIBS_RELEASE )
set(boost_boost__libboost_FRAMEWORK_DIRS_RELEASE )
set(boost_boost__libboost_FRAMEWORKS_RELEASE )
set(boost_boost__libboost_DEPENDENCIES_RELEASE Boost::headers)
set(boost_boost__libboost_SHARED_LINK_FLAGS_RELEASE )
set(boost_boost__libboost_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_boost__libboost_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_boost__libboost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_boost__libboost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_boost__libboost_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_boost__libboost_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_boost__libboost_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_boost__libboost_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::boost VARIABLES ############################################

set(boost_Boost_boost_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_boost_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_boost_RES_DIRS_RELEASE )
set(boost_Boost_boost_DEFINITIONS_RELEASE )
set(boost_Boost_boost_OBJECTS_RELEASE )
set(boost_Boost_boost_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_boost_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_boost_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_boost_LIBS_RELEASE )
set(boost_Boost_boost_SYSTEM_LIBS_RELEASE )
set(boost_Boost_boost_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_boost_FRAMEWORKS_RELEASE )
set(boost_Boost_boost_DEPENDENCIES_RELEASE Boost::headers)
set(boost_Boost_boost_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_boost_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_boost_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_boost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_boost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_boost_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_boost_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_boost_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_boost_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::headers VARIABLES ############################################

set(boost_Boost_headers_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_headers_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_headers_RES_DIRS_RELEASE )
set(boost_Boost_headers_DEFINITIONS_RELEASE )
set(boost_Boost_headers_OBJECTS_RELEASE )
set(boost_Boost_headers_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_headers_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_headers_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_headers_LIBS_RELEASE )
set(boost_Boost_headers_SYSTEM_LIBS_RELEASE )
set(boost_Boost_headers_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_headers_FRAMEWORKS_RELEASE )
set(boost_Boost_headers_DEPENDENCIES_RELEASE Boost::diagnostic_definitions Boost::disable_autolinking Boost::dynamic_linking)
set(boost_Boost_headers_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_headers_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_headers_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_headers_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_headers_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_headers_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_headers_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::dynamic_linking VARIABLES ############################################

set(boost_Boost_dynamic_linking_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_dynamic_linking_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_dynamic_linking_RES_DIRS_RELEASE )
set(boost_Boost_dynamic_linking_DEFINITIONS_RELEASE )
set(boost_Boost_dynamic_linking_OBJECTS_RELEASE )
set(boost_Boost_dynamic_linking_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_dynamic_linking_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_dynamic_linking_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_dynamic_linking_LIBS_RELEASE )
set(boost_Boost_dynamic_linking_SYSTEM_LIBS_RELEASE )
set(boost_Boost_dynamic_linking_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_dynamic_linking_FRAMEWORKS_RELEASE )
set(boost_Boost_dynamic_linking_DEPENDENCIES_RELEASE )
set(boost_Boost_dynamic_linking_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_dynamic_linking_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_dynamic_linking_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_dynamic_linking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_dynamic_linking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_dynamic_linking_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_dynamic_linking_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_dynamic_linking_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_dynamic_linking_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::disable_autolinking VARIABLES ############################################

set(boost_Boost_disable_autolinking_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_disable_autolinking_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_disable_autolinking_RES_DIRS_RELEASE )
set(boost_Boost_disable_autolinking_DEFINITIONS_RELEASE )
set(boost_Boost_disable_autolinking_OBJECTS_RELEASE )
set(boost_Boost_disable_autolinking_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_disable_autolinking_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_disable_autolinking_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_disable_autolinking_LIBS_RELEASE )
set(boost_Boost_disable_autolinking_SYSTEM_LIBS_RELEASE )
set(boost_Boost_disable_autolinking_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_disable_autolinking_FRAMEWORKS_RELEASE )
set(boost_Boost_disable_autolinking_DEPENDENCIES_RELEASE )
set(boost_Boost_disable_autolinking_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_disable_autolinking_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_disable_autolinking_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_disable_autolinking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_disable_autolinking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_disable_autolinking_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_disable_autolinking_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_disable_autolinking_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_disable_autolinking_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::diagnostic_definitions VARIABLES ############################################

set(boost_Boost_diagnostic_definitions_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_diagnostic_definitions_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_diagnostic_definitions_RES_DIRS_RELEASE )
set(boost_Boost_diagnostic_definitions_DEFINITIONS_RELEASE )
set(boost_Boost_diagnostic_definitions_OBJECTS_RELEASE )
set(boost_Boost_diagnostic_definitions_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_diagnostic_definitions_LIBS_RELEASE )
set(boost_Boost_diagnostic_definitions_SYSTEM_LIBS_RELEASE )
set(boost_Boost_diagnostic_definitions_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_diagnostic_definitions_FRAMEWORKS_RELEASE )
set(boost_Boost_diagnostic_definitions_DEPENDENCIES_RELEASE )
set(boost_Boost_diagnostic_definitions_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_diagnostic_definitions_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(boost_Boost_diagnostic_definitions_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_diagnostic_definitions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_diagnostic_definitions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_diagnostic_definitions_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_C_RELEASE}>")