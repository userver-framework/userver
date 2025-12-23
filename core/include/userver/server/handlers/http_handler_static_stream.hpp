#pragma once

/// @file userver/server/handlers/http_handler_static.hpp
/// @brief @copybrief server::handlers::HttpHandlerStaticStream

#include <userver/dynamic_config/source.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Streaming handler that returns HTTP 200 if file exist and returns file data with mapped content/type.
///
/// Path arguments of this handle are passed to `dir` property to get the file.
///
/// @code{.yaml}
/// handler-static-stream:
///     dir: /var/www          # Path to the directory with files
/// @endcode
///
/// the `handler-static-stream` with `path: /files/*` on request to `/files/some/file.html`
/// would return file at path `/var/www/some/file.html`.
///
/// ## HttpHandlerStaticStream Dynamic config
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
/// dir                | Base directory path                                                                       | /var/www
/// directory-file     | File to return for directory requests. File name (not path) search in requested directory | "index.html"
/// not-found-file     | File to return for missing files                                                          | "/404.html"
/// buffer-size        | Single read buffer size in bytes                                                          | 8192
///

// clang-format on

class HttpHandlerStaticStream final : public HttpHandlerBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::HttpHandlerStaticStream
    static constexpr std::string_view kName = "handler-static-stream";

    using HttpHandlerBase::HttpHandlerBase;

    HttpHandlerStaticStream(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    void HandleStreamRequest(http::HttpRequest&, request::RequestContext&, http::ResponseBodyStream&) const override;

private:
    dynamic_config::Source config_;
    const std::string base_dir_;
    const std::size_t buffer_size_;
    const std::string directory_file_;
    const std::string not_found_file_;
    engine::TaskProcessor& fs_task_processor_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
