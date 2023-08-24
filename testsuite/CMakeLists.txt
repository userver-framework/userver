configure_file(env.in env @ONLY)

if (USERVER_IS_THE_ROOT_PROJECT)
    add_subdirectory(tests)
endif()
