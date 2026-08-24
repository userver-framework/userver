#pragma once

/// @file userver/server/handlers/impl/pprof.hpp
/// @brief Building blocks of the `pprof` remote profiling protocol and of the
/// jemalloc control commands, shared by the handlers that implement them. Not a
/// part of the public API.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/handlers/jemalloc.hpp>
#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

/// @brief The maximal accepted size of a `symbol` POST body.
inline constexpr std::size_t kMaxPprofSymbolBodySize = 1024 * 1024;

/// @brief The maximal accepted count of distinct addresses in a `symbol` POST
/// body.
inline constexpr std::size_t kMaxPprofSymbolAddresses = 32768;

enum class PprofSymbolParseError { kInvalidAddress, kTooManyAddresses };

using PprofSymbolAddresses = std::vector<std::uintptr_t>;
using PprofSymbolParseResult = std::variant<PprofSymbolAddresses, PprofSymbolParseError>;

/// @brief Whether the request method is one that only reads.
bool IsPprofReadMethod(const http::HttpRequest& request);

/// @brief Sets the 405 status and returns the body describing it.
std::string PprofMethodNotAllowed(const http::HttpRequest& request);

/// @brief The `symbol` page answer for a read request: reports to `jeprof` that
/// symbolization is available.
std::string PprofSymbolGet();

/// @brief Parses the `+`-separated hex addresses of a `symbol` POST body,
/// preserving their order and dropping duplicates.
///
/// @returns An error if a token is not a nonzero hex address or if the body
/// holds more than kMaxPprofSymbolAddresses distinct addresses.
PprofSymbolParseResult ParsePprofSymbolAddresses(std::string_view body);

/// @brief Maps addresses to function names, one `0x<address> <name>` line per
/// resolved address.
std::string SymbolizePprofAddresses(const PprofSymbolAddresses& addresses);

/// @brief The `symbol` page answer for a POST request: maps the `+`-separated
/// hex addresses of the request body to function names, one
/// `0x<address> <name>` line per resolved address.
std::string PprofSymbolPost(const http::HttpRequest& request);

/// @brief Marks the response as a binary profile and returns the dump contents.
std::string PprofReadDump(
    const http::HttpRequest& request,
    engine::TaskProcessor& fs_task_processor,
    const std::string& dump_path
);

/// @brief Handles the allocator independent part of the `pprof` protocol.
///
/// @returns The response body of an allocator-independent command, the error
/// body of a read only command that was invoked with a modifying method, or
/// std::nullopt if the command has to be handled by the allocator specific code
/// of the caller.
std::optional<std::string> TryHandlePprofCommand(const http::HttpRequest& request, Jemalloc::Command command);

/// @brief Whether the `MALLOC_CONF` environment variable of the process enables
/// the jemalloc heap profiling.
bool IsJemallocProfilingEnabledViaEnv();

/// @brief Sets the 503 status and returns the body telling that the service was
/// started without the jemalloc heap profiling enabled.
std::string JemallocProfilingUnavailable(const http::HttpRequest& request);

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
