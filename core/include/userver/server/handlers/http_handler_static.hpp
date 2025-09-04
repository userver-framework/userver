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
/// @brief Handler that returns HTTP 200 if file exist and returns file data with mapped content/type.
///
/// Path arguments of this handle are passed to `fs-cache-component` to get the file. In other words, for the following
/// `fs-cache-main`:
/// @code{.yaml}
/// fs-cache-main:
///     dir: /fs-cache-main-path/          # Path to the directory with files
/// @endcode
/// the `handler-static` with `path: /handler-static-path/*` on request to `/handler-static-path/some/file.html`
/// would return file at path `/fs-cache-main-path/some/file.html`.
///
/// ## HttpHandlerStatic Dynamic config
/// * @ref USERVER_FILES_CONTENT_TYPE_MAP
///
/// \ref userver_http_handlers "Userver HTTP Handlers".
///
/// ## Static options:
/// Inherits all the options from server::handlers::HttpHandlerBase and adds the
/// following ones:
///
/// Name               | Description                                                                               | Default value
/// ------------------ | ----------------------------------------------------------------------------------------- | -------------
/// fs-cache-component | Name of the components::FsCache component                                                 | fs-cache-component
/// expires            | Cache age in seconds                                                                      | 600
/// directory-file     | File to return for directory requests. File name (not path) search in requested directory | "index.html"
/// not-found-file     | File to return for missing files                                                          | "/404.html"
///
/// ## Example usage:
///
/// @snippet samples/static_service/main.cpp Static service sample - main

// clang-format on

class HttpHandlerStatic final : public HttpHandlerBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::HttpHandlerStatic
    static constexpr std::string_view kName = "handler-static";

    using HttpHandlerBase::HttpHandlerBase;

    HttpHandlerStatic(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const http::HttpRequest& request, request::RequestContext&) const override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    dynamic_config::Source config_;
    const fs::FsCacheClient& storage_;
    const std::chrono::seconds cache_age_;
    const std::string directory_file_;
    const std::string not_found_file_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
