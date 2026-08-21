#pragma once

#include <string>
#include <string_view>

#include <userver/components/component.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <default_query_cache.hpp>
#include <runtime_query_cache.hpp>

namespace functional_tests {

/// @brief Reports the contents of both caches and how RuntimeQueryCache has
/// been updated, so that the tests can check them.
class CacheStateHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cache-state";

    CacheStateHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          runtime_query_cache_(context.FindComponent<RuntimeQueryCache>()),
          default_query_cache_(context.FindComponent<DefaultQueryCache>())
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

        formats::json::ValueBuilder response(formats::json::Type::kObject);
        response["runtime-query-cache"] = DumpCache(runtime_query_cache_);
        response["default-query-cache"] = DumpCache(default_query_cache_);
        response["make-find-operation-count"] = runtime_query_cache_.GetMakeFindOperationCount();
        response["last-update-type"] =
            runtime_query_cache_.GetLastUpdateType() == cache::UpdateType::kFull ? "full" : "incremental";
        return formats::json::ToString(response.ExtractValue());
    }

private:
    template <typename Cache>
    static formats::json::Value DumpCache(const Cache& cache) {
        const auto data = cache.Get();
        formats::json::ValueBuilder builder(formats::json::Type::kObject);
        for (const auto& [key, object] : *data) {
            builder[key] = object.value;
        }
        return builder.ExtractValue();
    }

    const RuntimeQueryCache& runtime_query_cache_;
    const DefaultQueryCache& default_query_cache_;
};

}  // namespace functional_tests
