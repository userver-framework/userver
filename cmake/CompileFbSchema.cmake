function(compile_fb_schema_to_cpp FB_GEN_TARGET FB_INCLUDE_DIRS)
  foreach(FB_SCHEME ${ARGN})
    set(FB_SCHEME_FULL "${CMAKE_CURRENT_SOURCE_DIR}/${FB_SCHEME}")

    string(REPLACE "/" ";" FB_SCHEME_PATH_PARTS "${FB_SCHEME}")
    list(GET FB_SCHEME_PATH_PARTS 0 FB_SCHEME_FIRST_PATH_PART)
    list(GET FB_SCHEME_PATH_PARTS 1 FB_SCHEME_SECOND_PATH_PART)

    set(FB_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${FB_SCHEME_FIRST_PATH_PART}")
    set(FB_INCLUDE_SUBDIR "${CMAKE_CURRENT_BINARY_DIR}/${FB_SCHEME_FIRST_PATH_PART}/${FB_SCHEME_SECOND_PATH_PART}")

    get_filename_component(FB_SCHEME_DIR "${FB_SCHEME}" PATH)

    set(FB_OUT_GEN_HDR_DIR "${CMAKE_CURRENT_BINARY_DIR}/${FB_SCHEME_DIR}")

    string(REGEX REPLACE "\\.fbs$" "_generated.h" FB_GEN_HDR "${FB_SCHEME}")

    set(FB_OUT_GEN_HDR "${CMAKE_CURRENT_BINARY_DIR}/${FB_GEN_HDR}")

    add_custom_command(
      PRE_BUILD
      OUTPUT "${FB_OUT_GEN_HDR}"
      COMMAND flatc --cpp --gen-object-api -o "${FB_OUT_GEN_HDR_DIR}" "${FB_SCHEME_FULL}"
      DEPENDS "${FB_SCHEME_FULL}")
    list(APPEND FB_GEN_HDRS "${FB_OUT_GEN_HDR}")
    list(APPEND FB_INCLUDE_DIRS "${FB_INCLUDE_DIR}")
    list(APPEND FB_INCLUDE_DIRS "${FB_INCLUDE_SUBDIR}")
  endforeach()

  add_custom_target("${FB_GEN_TARGET}" DEPENDS "${FB_GEN_HDRS}")
  set(${FB_INCLUDE_DIRS} ${${FB_INCLUDE_DIRS}} PARENT_SCOPE)
endfunction()

function(compile_fb_schema_to_py FB_GEN_TARGET BASE_PATH)
  foreach(FB_SCHEME ${ARGN})
    list(APPEND FB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${FB_SCHEME}")
  endforeach()

  add_custom_target("${FB_GEN_TARGET}"
    COMMAND flatc --python
        -o "${CMAKE_CURRENT_BINARY_DIR}/${BASE_PATH}" ${FB_FILES}
    COMMAND touch "${CMAKE_CURRENT_BINARY_DIR}/${BASE_PATH}/__init__.py"
    DEPENDS ${ARGN})
endfunction()
