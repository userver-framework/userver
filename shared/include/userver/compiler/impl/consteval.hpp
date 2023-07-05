#pragma once

#if __cpp_consteval >= 201811L || \
    ((__GNUC__ >= 10 || __clang_major__ >= 14) && __cplusplus >= 202002L)
#define USERVER_IMPL_CONSTEVAL consteval
#else
#define USERVER_IMPL_CONSTEVAL constexpr
#endif
