#pragma once

/// @file userver/server/handlers/http_handler_static.hpp
/// @brief @copybrief server::handlers::HttpHandlerStatic

#include <userver/components/fs_cache.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/fs/fs_cache_client.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that returns HTTP 200 if file exist
/// and returns file data with mapped content/type
///
/// ## Dynamic config
/// * @ref USERVER_FILES_CONTENT_TYPE_MAP
///
/// \ref userver_http_handlers "Userver HTTP Handlers".
///
/// ## Static options:
/// Inherits all the options from server::handlers::HttpHandlerBase and adds the
/// following ones:
///
/// Name               | Description                   | Default value
/// ------------------ | ----------------------------- | -------------
/// fs-cache-component | Name of the FsCache component | fs-cache-component
///
/// ## Example usage:
///
/// @snippet samples/static_service/static_service.cpp Static service sample - main

// clang-format on

class HttpHandlerStatic final : public HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-static";

  using HttpHandlerBase::HttpHandlerBase;

  HttpHandlerStatic(const components::ComponentConfig& config,
                    const components::ComponentContext& context);

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  dynamic_config::Source config_;
  const fs::FsCacheClient& storage_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
