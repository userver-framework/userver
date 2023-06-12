#pragma once

/// @file userver/logging/log_filepath.hpp
/// @brief Short source path calculator

#include <string_view>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

// I know no other way to wrap a macro value in quotes
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_LOG_FILEPATH_STRINGIZE_AUX(X) #X
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_LOG_FILEPATH_STRINGIZE(X) USERVER_LOG_FILEPATH_STRINGIZE_AUX(X)

// May have different macro values for different translation units, hence static
// TODO: consteval
static constexpr std::size_t PathBaseSize(std::string_view path) noexcept {
  for (const std::string_view base : {
#ifdef USERVER_LOG_PREFIX_PATH_BASE
           USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_PREFIX_PATH_BASE),
#endif
#ifdef USERVER_LOG_SOURCE_PATH_BASE
           USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_SOURCE_PATH_BASE),
#endif
#ifdef USERVER_LOG_BUILD_PATH_BASE
           USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_BUILD_PATH_BASE),
#endif
           ""  // default in case none were defined
       }) {
    if (path.substr(0, base.size()) == base) {
      std::size_t base_size = path.find_first_not_of('/', base.size());
      if (base_size == std::string_view::npos) {
        base_size = path.size();
      }

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
#define USERVER_FILEPATH                                                      \
  &__FILE__[std::integral_constant<size_t, USERVER_NAMESPACE::logging::impl:: \
                                               PathBaseSize(__FILE__)>::value]

USERVER_NAMESPACE_END
