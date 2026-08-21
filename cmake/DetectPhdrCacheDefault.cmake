include_guard(GLOBAL)

# Sets USERVER_DISABLE_PHDR_CACHE_DEFAULT in the caller's scope based on whether
# exception unwinding can use glibc's lock-free _dl_find_object. When available,
# userver's phdr cache is redundant (and interferes with dlopen).
#
# Default is ON when:
# - USERVER_SANITIZE is set (phdr cache / dl_iterate_phdr hooks conflict with
#   sanitizers), or
# - both of the following succeed:
#   1. Compiler version: GCC 12+ or Clang 15+ (unwinders that know about
#      _dl_find_object);
#   2. readelf on the actual libgcc_s / libunwind from -print-file-name= finds
#      _dl_find_object (confirms the linked runtime actually uses it).
#
# Only USERVER_DISABLE_PHDR_CACHE_DEFAULT is written to the caller's scope.
function(_userver_detect_phdr_cache_default)
    # USERVER_SANITIZE may already be in the cache from -DUSERVER_SANITIZE=...
    # even before cmake/Sanitizers.cmake runs.
    if(USERVER_SANITIZE)
        set(USERVER_DISABLE_PHDR_CACHE_DEFAULT
            ON
            PARENT_SCOPE
        )
        message(STATUS "phdr cache is disabled by default (sanitizers are enabled: ${USERVER_SANITIZE})")
        return()
    endif()

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(USERVER_DISABLE_PHDR_CACHE_DEFAULT
            OFF
            PARENT_SCOPE
        )
        return()
    endif()

    set(disable_phdr_cache_default OFF)

    set(compiler_has_dl_find_object OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # libgcc started using _dl_find_object in GCC 12.
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12")
            set(compiler_has_dl_find_object ON)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # LLVM libunwind gained _dl_find_object support around Clang 15.
        # On Linux Clang often links libgcc_s instead; readelf covers that.
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15")
            set(compiler_has_dl_find_object ON)
        endif()
    endif()

    set(unwinder_has_dl_find_object OFF)
    find_program(_userver_readelf_bin NAMES readelf)
    if(_userver_readelf_bin)
        foreach(unwinder_lib libgcc_s.so.1 libunwind.so.1 libunwind.so)
            execute_process(
                COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${unwinder_lib}
                OUTPUT_VARIABLE unwinder_lib_path
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
            )
            if(EXISTS "${unwinder_lib_path}" AND NOT IS_DIRECTORY "${unwinder_lib_path}")
                execute_process(
                    COMMAND ${_userver_readelf_bin} -sW "${unwinder_lib_path}"
                    OUTPUT_VARIABLE unwinder_symbols
                    ERROR_QUIET
                )
                string(FIND "${unwinder_symbols}" "_dl_find_object" dl_find_object_pos)
                if(NOT dl_find_object_pos EQUAL -1)
                    set(unwinder_has_dl_find_object ON)
                    break()
                endif()
            endif()
        endforeach()
    else()
        message(STATUS "readelf not found; cannot inspect unwinder for _dl_find_object")
    endif()

    if(compiler_has_dl_find_object AND unwinder_has_dl_find_object)
        set(disable_phdr_cache_default ON)
        message(STATUS "phdr cache is disabled by default "
                       "(${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} and unwinder "
                       "support _dl_find_object)"
        )
    else()
        set(phdr_reason "")
        if(NOT compiler_has_dl_find_object)
            string(APPEND phdr_reason "compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} is older than "
                   "GCC 12 / Clang 15"
            )
        endif()
        if(NOT unwinder_has_dl_find_object)
            if(phdr_reason)
                string(APPEND phdr_reason "; ")
            endif()
            string(APPEND phdr_reason "_dl_find_object not found in unwinder")
        endif()
        message(STATUS "phdr cache is enabled by default (${phdr_reason})")
    endif()

    set(USERVER_DISABLE_PHDR_CACHE_DEFAULT
        ${disable_phdr_cache_default}
        PARENT_SCOPE
    )
endfunction()
