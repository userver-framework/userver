#include <userver/server/handlers/impl/pprof.hpp>

#include <charconv>
#include <iterator>
#include <optional>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

#include <userver/fs/read.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/assert.hpp>
#include <utils/jemalloc.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

namespace {

constexpr std::string_view kNumSymbols = "num_symbols: 1\n";

constexpr std::string_view kHexPrefix = "0x";

std::optional<std::uintptr_t> ParseHexAddress(std::string_view token) {
    const auto prefix = token.substr(0, kHexPrefix.size());
    if (token.size() > kHexPrefix.size() && (prefix == "0x" || prefix == "0X")) {
        token.remove_prefix(kHexPrefix.size());
    }

    std::uintptr_t address = 0;
    const auto* const token_begin = token.data();
    const auto* const token_end = token_begin + token.size();
    const auto [parse_end, error_code] = std::from_chars(token_begin, token_end, address, 16);
    if (error_code != std::errc{} || parse_end != token_end) {
        return std::nullopt;
    }
    return address;
}

}  // namespace

bool IsPprofReadMethod(const http::HttpRequest& request) { return request.GetMethod() == http::HttpMethod::kGet; }

std::string PprofMethodNotAllowed(const http::HttpRequest& request) {
    request.SetResponseStatus(server::http::HttpStatus::kMethodNotAllowed);
    return fmt::format("Unsupported method for this command: {}\n", request.GetMethodStr());
}

std::string PprofSymbolGet() { return std::string{kNumSymbols}; }

PprofSymbolParseResult ParsePprofSymbolAddresses(std::string_view body) {
    PprofSymbolAddresses addresses;
    std::unordered_set<std::uintptr_t> seen;

    for (std::size_t pos = 0; pos <= body.size();) {
        const auto separator = body.find('+', pos);
        const auto end = (separator == std::string_view::npos) ? body.size() : separator;
        const auto address = ParseHexAddress(body.substr(pos, end - pos));

        if (!address.has_value() || address.value() == 0) {
            return PprofSymbolParseError::kInvalidAddress;
        }

        if (seen.insert(address.value()).second) {
            if (addresses.size() >= kMaxPprofSymbolAddresses) {
                return PprofSymbolParseError::kTooManyAddresses;
            }
            addresses.push_back(address.value());
        }

        if (separator == std::string_view::npos) {
            break;
        }
        pos = separator + 1;
    }

    return addresses;
}

std::string SymbolizePprofAddresses(const PprofSymbolAddresses& addresses) {
    std::string result;
    for (const auto address : addresses) {
        const auto name = logging::stacktrace_cache::SymbolizeAddress(reinterpret_cast<const void*>(address));

        if (!name.empty()) {
            fmt::format_to(std::back_inserter(result), "0x{:x} {}\n", address, name);
        }
    }
    return result;
}

std::string PprofSymbolPost(const http::HttpRequest& request) {
    const std::string_view body = request.RequestBody();
    if (body.size() > kMaxPprofSymbolBodySize) {
        request.SetResponseStatus(server::http::HttpStatus::kPayloadTooLarge);
        return fmt::format("Request body is too large: {} > {} bytes\n", body.size(), kMaxPprofSymbolBodySize);
    }

    const auto parse_result = ParsePprofSymbolAddresses(body);
    if (const auto* error = std::get_if<PprofSymbolParseError>(&parse_result)) {
        if (*error == PprofSymbolParseError::kInvalidAddress) {
            request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
            return "Request body contains an invalid address\n";
        }
        UASSERT(*error == PprofSymbolParseError::kTooManyAddresses);
        request.SetResponseStatus(server::http::HttpStatus::kPayloadTooLarge);
        return fmt::format("Too many addresses requested, the limit is {}\n", kMaxPprofSymbolAddresses);
    }

    return SymbolizePprofAddresses(std::get<PprofSymbolAddresses>(parse_result));
}

std::string PprofReadDump(
    const http::HttpRequest& request,
    engine::TaskProcessor& fs_task_processor,
    const std::string& dump_path
) {
    request.GetHttpResponse().SetContentType(USERVER_NAMESPACE::http::content_type::kApplicationOctetStream);
    return fs::ReadFileContents(fs_task_processor, dump_path);
}

std::optional<std::string> TryHandlePprofCommand(const http::HttpRequest& request, Jemalloc::Command command) {
    switch (command) {
        case Jemalloc::Command::kSymbol:
            if (request.GetMethod() == http::HttpMethod::kPost) {
                return PprofSymbolPost(request);
            }
            if (!IsPprofReadMethod(request)) {
                return PprofMethodNotAllowed(request);
            }
            return PprofSymbolGet();
        case Jemalloc::Command::kHeap:
            if (!IsPprofReadMethod(request)) {
                return PprofMethodNotAllowed(request);
            }
            return std::nullopt;
        default:
            return std::nullopt;
    }
}

bool IsJemallocProfilingEnabledViaEnv() { return utils::jemalloc::IsProfilingEnabledViaEnv(); }

std::string JemallocProfilingUnavailable(const http::HttpRequest& request) {
    request.SetResponseStatus(server::http::HttpStatus::kServiceUnavailable);
    return "'jemalloc' profiling is not available because the service was not started with a 'MALLOC_CONF' "
           "environment variable that contain 'prof:true'\n";
}

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
