#include "http_server_logs.hpp"

#include <algorithm>
#include <exception>
#include <optional>
#include <utility>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/handlers/json_body_logging.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>

#include <dynamic_config/variables/USERVER_HTTP_SERVER_LOGS.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {
namespace {

using MaskingPolicy = ::dynamic_config::userver_http_server_logs::BodyMasking;
using GeneratedListMode = decltype(MaskingPolicy::list_mode);

formats::json::MaskingListMode ConvertListMode(GeneratedListMode mode) {
    switch (mode) {
        case GeneratedListMode::kBlacklist:
            return formats::json::MaskingListMode::kBlacklist;
        case GeneratedListMode::kWhitelist:
            return formats::json::MaskingListMode::kWhitelist;
    }
    UINVARIANT(false, "Unexpected HTTP body masking list mode");
}

std::optional<formats::json::MaskingRules> CompileMasking(const std::optional<MaskingPolicy>& policy) {
    if (!policy) {
        return std::nullopt;
    }
    return formats::json::MaskingRules{policy->paths, ConvertListMode(policy->list_mode)};
}

HttpServerLogs ParseHttpServerLogs(const dynamic_config::DocsMap& docs) {
    const auto settings = ::dynamic_config::USERVER_HTTP_SERVER_LOGS.Parse(docs);
    HttpServerLogs result;
    for (const auto& [path, limits] : settings.extra) {
        result.emplace(
            path,
            HttpLoggingSettings{
                {limits.request_body_size_log_limit, CompileMasking(limits.json_request_body_masking)},
                {limits.response_body_size_log_limit, CompileMasking(limits.json_response_body_masking)}
            }
        );
    }
    return result;
}

}  // namespace

const dynamic_config::Key<HttpServerLogs> kHttpServerLogs{dynamic_config::impl::InternalTag{}, &ParseHttpServerLogs};

std::string MaskBodyForLogging(std::string_view body, const formats::json::MaskingRules& rules, std::size_t limit) {
    if (body.empty()) {
        return {};
    }
    try {
        auto result = formats::json::MaskJson(body, rules, limit);
        if (result.limit_reached) {
            const auto suffix = fmt::format(FMT_COMPILE("...(truncated, total {} bytes)"), body.size());
            if (suffix.size() > limit) {
                return suffix.substr(0, limit);
            }
            result.text.resize(std::min(result.text.size(), limit - suffix.size()));
            utils::text::utf8::TrimTruncatedEnding(result.text);
            result.text += suffix;
        }
        return std::move(result.text);
    } catch (const std::exception&) {
        return "<error masking JSON body>";
    }
}

}  // namespace server::handlers::impl

namespace server::handlers {

namespace {

const impl::HttpLoggingSettings* FindLoggingSettings(
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config
) {
    return utils::FindOrNullptr(snapshot[impl::kHttpServerLogs], config.GetLowCardinalityRequestPath());
}

template <typename String>
std::string MaskJsonBodyForLogging(
    String&& body,
    const impl::HttpBodyLoggingSettings* settings,
    std::size_t static_limit
) {
    if (!settings || !settings->masking) {
        return body;
    }
    return impl::MaskBodyForLogging(body, *settings->masking, settings->limit.value_or(static_limit));
}

}  // namespace

std::string MaskJsonRequestBodyForLogging(
    const std::string& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config
) {
    const auto* settings = FindLoggingSettings(snapshot, config);
    return MaskJsonBodyForLogging(body, settings ? &settings->request : nullptr, config.request_body_size_log_limit);
}

std::string MaskJsonRequestBodyForLogging(
    std::string&& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config
) {
    const auto* settings = FindLoggingSettings(snapshot, config);
    return MaskJsonBodyForLogging(
        std::move(body),
        settings ? &settings->request : nullptr,
        config.request_body_size_log_limit
    );
}

std::string MaskJsonResponseBodyForLogging(
    const std::string& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config
) {
    const auto* settings = FindLoggingSettings(snapshot, config);
    return MaskJsonBodyForLogging(body, settings ? &settings->response : nullptr, config.response_data_size_log_limit);
}

std::string MaskJsonResponseBodyForLogging(
    std::string&& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config
) {
    const auto* settings = FindLoggingSettings(snapshot, config);
    return MaskJsonBodyForLogging(
        std::move(body),
        settings ? &settings->response : nullptr,
        config.response_data_size_log_limit
    );
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
