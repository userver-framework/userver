#pragma once

#ifdef ARCADIA_ROOT

#ifdef DEFAULT_DYNAMIC_CONFIG_SUFFIX

#include <library/cpp/testing/common/env.h>

#define DEFAULT_DYNAMIC_CONFIG_FILENAME \
  ArcadiaSourceRoot() + DEFAULT_DYNAMIC_CONFIG_SUFFIX

#endif

#endif
