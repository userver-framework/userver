#pragma once

#include <string_view>

#include <spdlog/version.h>

// this header must be included before any spdlog headers
// to override spdlog's level names
#ifdef SPDLOG_LEVEL_NAMES
#error "logging/log_config.hpp must be included before any spdlog headers!"
#endif
// spdlog level names
// Intentionally not wrapped in ifndef -- to show places where the order is
// incorrect
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_LEVEL_NAMES                                       \
  {                                                              \
    std::string_view{"TRACE"}, std::string_view{"DEBUG"},        \
        std::string_view{"INFO"}, std::string_view{"WARNING"},   \
        std::string_view{"ERROR"}, std::string_view{"CRITICAL"}, \
        std::string_view{"OFF"},                                 \
  }

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
