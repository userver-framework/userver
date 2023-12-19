#pragma once

#if __clang_major__ >= 13
#define USERVER_IMPL_LIFETIME_BOUND [[clang::lifetimebound]]
#else
#define USERVER_IMPL_LIFETIME_BOUND
#endif
