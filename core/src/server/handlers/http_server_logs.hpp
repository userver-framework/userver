#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json/masking.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

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

using HttpServerLogs = utils::impl::TransparentMap<std::string, HttpLoggingSettings>;

extern const dynamic_config::Key<HttpServerLogs> kHttpServerLogs;

std::string MaskBodyForLogging(std::string_view body, const formats::json::MaskingRules& rules, std::size_t limit);

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
