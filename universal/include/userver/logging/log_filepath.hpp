#pragma once

/// @file userver/logging/log_filepath.hpp
/// @brief Short source path calculator

#include <cstddef>
#include <string_view>

// TODO remove extra includes.
#include <fmt/format.h>
#include <concepts>
#include <string>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

#if !defined(USERVER_LOG_PREFIX_PATH_BASE) && !defined(USERVER_LOG_SOURCE_PATH_BASE) && \
    !defined(USERVER_LOG_BUILD_PATH_BASE)

/// @ingroup userver_universal
///
/// @brief Short `std::string_view` with source path for logging.
/// @hideinitializer
// We need user's filename here, not ours
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_FILEPATH std::string_view(__builtin_FILE())

#else

namespace logging::impl {

// I know no other way to wrap a macro value in quotes
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_LOG_FILEPATH_STRINGIZE_AUX(X) #X
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_LOG_FILEPATH_STRINGIZE(X) USERVER_LOG_FILEPATH_STRINGIZE_AUX(X)

// May have different macro values for different translation units, hence static
static constexpr std::size_t PathBaseSize(std::string_view path) noexcept {
    constexpr std::string_view kSourcePathPrefixes[] = {
#ifdef USERVER_LOG_PREFIX_PATH_BASE
        USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_PREFIX_PATH_BASE),
#endif
#ifdef USERVER_LOG_SOURCE_PATH_BASE
        USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_SOURCE_PATH_BASE),
#endif
#ifdef USERVER_LOG_BUILD_PATH_BASE
        USERVER_LOG_FILEPATH_STRINGIZE(USERVER_LOG_BUILD_PATH_BASE),
#endif
    };

    for (const std::string_view base : kSourcePathPrefixes) {
        if (path.starts_with(base)) {
            std::size_t base_size = path.find_first_not_of('/', base.size());
            if (base_size == std::string_view::npos) {
                base_size = path.size();
            }

            return base_size;
        }
    }
    return 0;
}

// May have different macro values for different translation units, hence static. `consteval` breaks the behavior of
// utils::SourceLocation.
static constexpr std::string_view CutFilePath(std::string_view path_view) noexcept {
    path_view.remove_prefix(impl::PathBaseSize(path_view));
    return path_view;
}

#undef USERVER_LOG_FILEPATH_STRINGIZE
#undef USERVER_LOG_FILEPATH_STRINGIZE_AUX

}  // namespace logging::impl

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_FILEPATH USERVER_NAMESPACE::logging::impl::CutFilePath(__builtin_FILE())

#endif

USERVER_NAMESPACE_END
