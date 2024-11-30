cmake_policy(SET CMP0053 NEW)

set(NAMESPACE userver)
set(FILE_IN ${CMAKE_CURRENT_BINARY_DIR}/embedded.h.in)
set(TEMPLATE "
#pragma once

#include <string_view>

#include <userver/utils/resources.hpp>

__asm__(R\"(
.section .rodata
.align 16
@NAME@_begin:
.incbin \"${FILEPATH}\"
@NAME@_end:
.byte 0
@NAME@_size:
.int @NAME@_end - @NAME@_begin
.section .text
)\");

extern \"C\" const char @NAME@_begin[];
extern \"C\" const char @NAME@_end;
extern \"C\" const int @NAME@_size;


__attribute__((constructor)) void @NAME@_call() {
  utils::RegisterResource(\"@NAME@\", std::string_view{@NAME@_begin, static_cast<size_t>(@NAME@_size)});
}
")
string(CONFIGURE "${TEMPLATE}" RENDERED)
file(WRITE "${OUTPUT}" "${RENDERED}")
