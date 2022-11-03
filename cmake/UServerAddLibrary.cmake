function(userver_add_library NAME)

  set(options)
  set(one_value_args)
  set(multi_value_args
    SOURCES
    PUBLIC_LINK_LIBRARIES
    PRIVATE_LINK_LIBRARIES
    )
  cmake_parse_arguments(USERVER_ADD_LIBRARY "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if (BUILD_SHARED_LIBS)
    add_library(${NAME} SHARED ${USERVER_ADD_LIBRARY_SOURCES})

    set_property(TARGET "${NAME}" PROPERTY POSITION_INDEPENDENT_CODE 1)

    target_link_libraries(${NAME}
      PUBLIC
        ${USERVER_ADD_LIBRARY_PUBLIC_LINK_LIBRARIES}
        ${USERVER_ADD_LIBRARY_PRIVATE_LINK_LIBRARIES}
        )
  else()
    add_library(${NAME} STATIC ${USERVER_ADD_LIBRARY_SOURCES})

    target_link_libraries(${NAME}
      PUBLIC
        ${USERVER_ADD_LIBRARY_PUBLIC_LINK_LIBRARIES}
      PRIVATE
        ${USERVER_ADD_LIBRARY_PRIVATE_LINK_LIBRARIES}
      )
  endif()

endfunction()

function(userver_target_link_libraries NAME)
  set(options)
  set(one_value_args)
  set(multi_value_args
    PUBLIC
    PRIVATE
    )
  cmake_parse_arguments(USERVER_ADD_LIBRARY "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if (BUILD_SHARED_LIBS)
    target_link_libraries(${NAME}
      PUBLIC
        ${USERVER_ADD_LIBRARY_PUBLIC}
        ${USERVER_ADD_LIBRARY_PRIVATE}
        )
  else()
    target_link_libraries(${NAME}
      PUBLIC
        ${USERVER_ADD_LIBRARY_PUBLIC}
      PRIVATE
        ${USERVER_ADD_LIBRARY_PRIVATE}
      )
  endif()
  
endfunction()
