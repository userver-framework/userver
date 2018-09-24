#pragma once

// this header must be included before any spdlog headers
// to override spdlog's level names
#ifdef SPDLOG_LEVEL_NAMES
#error "logging/log_config.hpp must be included before any spdlog headers!"
#endif
// spdlog level names
// Intentionally not wrapped in ifndef -- to show places where the order is
// incorrect
#define SPDLOG_LEVEL_NAMES \
  { "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "OFF" }

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
