cmake_policy(SET CMP0053 NEW)

set(NAMESPACE userver)
set(FILE_IN ${CMAKE_CURRENT_BINARY_DIR}/embedded.h.in)
set(TEMPLATE
    "
#pragma once

#include <string_view>

#include <userver/utils/resources.hpp>

#if defined(__APPLE__)
#define APPLE_PREFIX \"_\"
#else
#define APPLE_PREFIX \"\"
#endif

__asm__(
#if defined(__APPLE__)
\".const_data\\n\"
\".global _@NAME@_start\\n\"
\".global _@NAME@_end\\n\"
\".global _@NAME@_size\\n\"
#else
\".section .rodata\\n\"
#endif
\".balign 16\\n\"
APPLE_PREFIX \"@NAME@_begin:\\n\"
R\"(
.incbin \"${FILEPATH}\"
)\"
\".balign 1\\n\"
APPLE_PREFIX \"@NAME@_end:\\n\"
\".byte 0\\n\"
APPLE_PREFIX \"@NAME@_size:\\n\"
\".int \" APPLE_PREFIX \"@NAME@_end - \" APPLE_PREFIX \"@NAME@_begin\\n\"
);

extern \"C\" const char @NAME@_begin[];
extern \"C\" const char @NAME@_end;
extern \"C\" const int @NAME@_size;


__attribute__((constructor)) void @NAME@_call() {
  utils::RegisterResource(\"@NAME@\", std::string_view{@NAME@_begin, static_cast<size_t>(@NAME@_size)});
}
"
)
string(CONFIGURE "${TEMPLATE}" RENDERED)
file(WRITE "${OUTPUT}" "${RENDERED}")
