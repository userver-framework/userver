#pragma once

// Old clang-format behaves poorly with class attributes, use macro instead
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_NODISCARD [[nodiscard]]

// Old clang-format behaves poorly with deprecated functions, use macro instead
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_DEPRECATED(msg) [[deprecated(msg)]]
