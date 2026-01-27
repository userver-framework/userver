#pragma once

/// @file userver/server/handlers/dump_coroutines.hpp
/// @brief @copybrief server::handlers::DumpCoroutines

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that returns detailed alive coroutines information.
class DumpCoroutines final : public HttpHandlerJsonBase {
public:
    DumpCoroutines(const components::ComponentConfig& config, const components::ComponentContext& component_context);

    ~DumpCoroutines() override;

    /// @ingroup userver_component_names
    /// @brief The default name of server::handlers::DumpCoroutines
    static constexpr std::string_view kName = "handler-dump-coroutines";

    formats::json::Value HandleRequestJsonThrow(
        const http::HttpRequest& request,
        const formats::json::Value& request_json,
        request::RequestContext& context
    ) const override;

private:
    struct Impl;
    utils::FastPimpl<Impl, 8, 8> impl_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
