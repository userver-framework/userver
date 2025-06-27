#include <userver/server/handlers/http_handler_static.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <dynamic_config/variables/USERVER_FILES_CONTENT_TYPE_MAP.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

HttpHandlerStatic::HttpHandlerStatic(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : HttpHandlerBase(config, context),
      config_(context.FindComponent<components::DynamicConfig>().GetSource()),
      storage_(
          context.FindComponent<components::FsCache>(config["fs-cache-component"].As<std::string>("fs-cache-component"))
              .GetClient()
      ),
      cache_age_(config["expires"].As<std::chrono::seconds>(600)) {}

std::string HttpHandlerStatic::HandleRequestThrow(const http::HttpRequest& request, request::RequestContext&) const {
    std::string search_path;
    search_path.reserve(request.GetRequestPath().size());

    for (std::size_t i = 0; i < request.PathArgCount(); ++i) {
        auto& arg = request.GetPathArg(i);
        search_path += "/";
        search_path += arg;
    }

    LOG_DEBUG() << "search_path: " << search_path;

    auto& response = request.GetHttpResponse();
    const auto file = storage_.TryGetFile(search_path);
    if (file) {
        const auto config = config_.GetSnapshot();
        response.SetHeader(USERVER_NAMESPACE::http::headers::kExpires, std::to_string(cache_age_.count()));
        response.SetContentType(config[::dynamic_config::USERVER_FILES_CONTENT_TYPE_MAP][file->extension]);
        return file->data;
    }
    response.SetStatusNotFound();
    return "File not found";
}

yaml_config::Schema HttpHandlerStatic::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: |
    Handler that returns HTTP 200 if file exist
    and returns file data with mapped content/type
additionalProperties: false
properties:
    fs-cache-component:
        type: string
        description: Name of the FsCache component
        defaultDescription: fs-cache-component
    expires:
        type: string
        description: Cache age in seconds
        defaultDescription: 600
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
