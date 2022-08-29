#pragma once

#include <fmt/core.h>

#if FMT_VERSION < 80000
#define USERVER_FMT_CONST
#else
#define USERVER_FMT_CONST const
#endif
