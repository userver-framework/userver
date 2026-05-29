#pragma once

#include <pg_config.h>

#ifdef ARCADIA_ROOT
#define USERVER_LIBPQ_VERSION PG_VERSION_NUM
#else
#include <userver_libpq_version.hpp>  // Y_IGNORE
#endif
