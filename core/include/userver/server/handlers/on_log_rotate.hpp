#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class Logging;
}  // namespace components

namespace server::handlers {
class OnLogRotate final : public HttpHandlerBase {
 public:
  OnLogRotate(const components::ComponentConfig& config,
              const components::ComponentContext& component_context);

  static constexpr std::string_view kName = "handler-on-log-rotate";

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  components::Logging& logging_component_;
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::OnLogRotate> =
    true;

USERVER_NAMESPACE_END
