#pragma once

// avoid clashing symbols for osx
#if defined(__APPLE__) && defined(EV_ERROR)
constexpr inline const auto ev_error_workaround = EV_ERROR;
#undef EV_ERROR
#include <ev.h>
#define EV_ERROR ev_error_workaround
#else
#include <ev.h>
#endif
