#pragma once

#include <string>

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
}

#else  // NDEBUG

#define UASSERT(expr) \
  do {                \
  } while (0)

#define UASSERT_MSG(expr, msg) \
  do {                         \
  } while (0)

#endif  // NDEBUG
