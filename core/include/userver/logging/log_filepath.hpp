#pragma once

/// @file userver/logging/log_filepath.hpp
/// @brief Short source path calculator

#include <string_view>
#include <type_traits>

namespace logging::impl {

// I know no other way to wrap a macro value in quotes
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_LOG_FILEPATH_STRINGIZE_AUX(X) #X
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_LOG_FILEPATH_STRINGIZE(X) USERVER_LOG_FILEPATH_STRINGIZE_AUX(X)

// May have different macro values for different translation units, hence static
// TODO: consteval
static constexpr size_t PathBaseSize(std::string_view path) {
  for (const std::string_view base : {
#ifdef USERVER_LOG_BUILD_PATH_BASE
           USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_BUILD_PATH_BASE),
#endif
#ifdef USERVER_LOG_SOURCE_PATH_BASE
           USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_SOURCE_PATH_BASE),
#endif
           ""  // default in case none were defined
       }) {
    if (path.substr(0, base.size()) == base) {
      size_t base_size = base.size();
      while (base_size < path.size() && path[base_size] == '/') ++base_size;
      return base_size;
    }
  }
  return 0;
}

#undef USERVER_LOG_FILEPATH_STRINGIZE
#undef USERVER_LOG_FILEPATH_STRINGIZE_AUX

}  // namespace logging::impl

/// @brief Short source path for logging.
/// @hideinitializer
// We need user's filename here, not ours
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_FILEPATH             \
  std::string_view{__FILE__}.substr( \
      std::integral_constant<size_t, \
                             ::logging::impl::PathBaseSize(__FILE__)>::value)
