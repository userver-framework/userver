project(userver-uboost-coro)

include(Sanitizers)

include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(UBOOST_CORO_USE_UCONTEXT
  "Use ucontext for context switching"
  OFF "NOT USERVER_SANITIZE"
  ON # forced on USERVER_SANITIZE
)
if(UBOOST_CORO_USE_UCONTEXT)
  message(STATUS "Context impl: ucontext")
else()
  message(STATUS "Context impl: fcontext")
endif()

set(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/context/posix/stack_traits.cpp
)

enable_language(ASM)
if(APPLE)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^amd64")
    list(APPEND SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/jump_x86_64_sysv_macho_gas.S
      ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/make_x86_64_sysv_macho_gas.S
      ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/ontop_x86_64_sysv_macho_gas.S
    )
  else()
    list(APPEND SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/jump_arm64_aapcs_macho_gas.S
      ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/make_arm64_aapcs_macho_gas.S
      ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/ontop_arm64_aapcs_macho_gas.S
    )
  endif()
else() # Unix
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^amd64")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/jump_i386_sysv_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/make_i386_sysv_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/ontop_i386_sysv_elf_gas.S
      )
    else()
      list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/jump_x86_64_sysv_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/make_x86_64_sysv_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/ontop_x86_64_sysv_elf_gas.S
      )
    endif()
  else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/jump_arm_aapcs_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/make_arm_aapcs_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/ontop_arm_aapcs_elf_gas.S
      )
    else()
      list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/jump_arm64_aapcs_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/make_arm64_aapcs_elf_gas.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context/asm/ontop_arm64_aapcs_elf_gas.S
      )
    endif()
  endif()
endif()

if(UBOOST_CORO_USE_UCONTEXT)
  list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/context/continuation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/context/fiber.cpp
  )
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(${PROJECT_NAME} PRIVATE "-w") # no warnings
target_compile_definitions(${PROJECT_NAME} PRIVATE "BOOST_CONTEXT_SOURCE")

if (SANITIZE_ASAN_ENABLED)
  target_compile_definitions(${PROJECT_NAME} PUBLIC "BOOST_USE_ASAN")
endif()

if (SANITIZE_TSAN_ENABLED)
  target_compile_definitions(${PROJECT_NAME} PUBLIC "BOOST_USE_TSAN")
endif()

if(UBOOST_CORO_USE_UCONTEXT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC "BOOST_USE_UCONTEXT")
  if(MACOS)
    # *context functions are deprecated on macos
    target_compile_options(${PROJECT_NAME} PUBLIC "-Wno-error=deprecated-declarations")
    # for full ucontext definition
    target_compile_definitions(${PROJECT_NAME} PUBLIC "_XOPEN_SOURCE=600")
    # to retain non-POSIX extensions
    target_compile_definitions(${PROJECT_NAME} PUBLIC "_DARWIN_C_SOURCE")
  endif()
endif()

find_package(Boost REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)
