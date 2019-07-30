#pragma once

#include <string>

#include <fmt/format.h>

#include <utils/traceful_exception.hpp>

#ifndef NDEBUG

#define UASSERT(expr)                                                   \
  do {                                                                  \
    if (!(expr)) {                                                      \
      ::utils::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, ""); \
    }                                                                   \
  } while (0)

#define UASSERT_MSG(expr, msg)                                           \
  do {                                                                   \
    if (!(expr)) {                                                       \
      ::utils::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, msg); \
    }                                                                    \
  } while (0)

namespace utils {

[[noreturn]] void UASSERT_failed(const std::string& expr, const char* file,
                                 unsigned int line, const char* function,
                                 const std::string& msg) noexcept;
}  // namespace utils

#else  // NDEBUG

#define UASSERT(expr) \
  do {                \
  } while (0)

#define UASSERT_MSG(expr, msg) \
  do {                         \
  } while (0)

#endif  // NDEBUG

namespace utils {

class InvariantError : public TracefulException {
  using TracefulException::TracefulException;
};

[[noreturn]] void LogAndThrowInvariantError(const std::string& error);

#define YTX_INVARIANT(condition, message)                                     \
  do {                                                                        \
    if (!(condition)) {                                                       \
      const auto err_str =                                                    \
          ::fmt::format("Invariant ({}) violation: {}", #condition, message); \
      UASSERT_MSG(false, err_str);                                            \
      ::utils::LogAndThrowInvariantError(err_str);                            \
    }                                                                         \
  } while (0)

}  // namespace utils
