#pragma once

/// @file userver/alert/handler.hpp
/// @brief @copybrief alert::Handler

#include <userver/alerts/storage.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that returns the list of active fired alerts.
class Handler final : public server::handlers::HttpHandlerJsonBase {
 public:
  Handler(const components::ComponentConfig& config,
          const components::ComponentContext& context);

  /// @ingroup userver_component_names
  /// @brief The default name of alerts::Handler
  static constexpr std::string_view kName = "handler-fired-alerts";

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;

 private:
  Storage& storage_;
};

}  // namespace alerts

template <>
inline constexpr bool components::kHasValidate<alerts::Handler> = true;

USERVER_NAMESPACE_END
