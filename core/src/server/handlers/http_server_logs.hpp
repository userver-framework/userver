#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json/masking.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

struct HttpBodyLoggingSettings final {
    std::optional<std::size_t> limit;
    std::optional<formats::json::MaskingRules> masking;
};

struct HttpLoggingSettings final {
    HttpBodyLoggingSettings request;
    HttpBodyLoggingSettings response;
};

using HttpServerLogs = std::unordered_map<std::string, HttpLoggingSettings>;

extern const dynamic_config::Key<HttpServerLogs> kHttpServerLogs;

std::string MaskBodyForLogging(std::string_view body, const formats::json::MaskingRules& rules, std::size_t limit);

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
