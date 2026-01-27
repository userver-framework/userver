cmake_policy(SET CMP0053 NEW)

set(NAMESPACE USERVER_NAMESPACE)
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
\".global _@NAME_C_ESCAPED@_start\\n\"
\".global _@NAME_C_ESCAPED@_end\\n\"
\".global _@NAME_C_ESCAPED@_size\\n\"
#else
\".section .rodata\\n\"
#endif
\".balign 16\\n\"
APPLE_PREFIX \"@NAME_C_ESCAPED@_begin:\\n\"
R\"(
.incbin \"${FILEPATH}\"
)\"
\".balign 1\\n\"
APPLE_PREFIX \"@NAME_C_ESCAPED@_end:\\n\"
\".byte 0\\n\"
APPLE_PREFIX \"@NAME_C_ESCAPED@_size:\\n\"
\".int \" APPLE_PREFIX \"@NAME_C_ESCAPED@_end - \" APPLE_PREFIX \"@NAME_C_ESCAPED@_begin\\n\"
\".previous\\n\"
);

extern \"C\" const char @NAME_C_ESCAPED@_begin[];
extern \"C\" const char @NAME_C_ESCAPED@_end;
extern \"C\" const int @NAME_C_ESCAPED@_size;


inline __attribute__((constructor)) void @NAME_C_ESCAPED@_call() {
  ${NAMESPACE}::utils::RegisterResource(\"@NAME@\", std::string_view{@NAME_C_ESCAPED@_begin, static_cast<size_t>(@NAME_C_ESCAPED@_size)});
}
"
)
string(CONFIGURE "${TEMPLATE}" RENDERED)
file(WRITE "${OUTPUT}" "${RENDERED}")
