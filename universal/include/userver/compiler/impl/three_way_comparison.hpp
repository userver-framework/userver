#pragma once

#if defined(__has_include)
#if __has_include(<version>)

#include <version>
#if defined(__cpp_lib_three_way_comparison) || defined(ARCADIA_ROOT)
#include <compare>
#define USERVER_IMPL_HAS_THREE_WAY_COMPARISON
#endif

#endif
#endif
