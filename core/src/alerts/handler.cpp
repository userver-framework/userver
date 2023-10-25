#include <userver/alerts/handler.hpp>

#include <userver/alerts/component.hpp>
#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

Handler::Handler(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context, true),
      storage_(context.FindComponent<StorageComponent>().GetStorage()) {}

formats::json::Value Handler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value&,
    server::request::RequestContext&) const {
  formats::json::ValueBuilder builder(formats::json::Type::kArray);

  for (const auto& alert : storage_.CollectActiveAlerts()) {
    builder.PushBack(formats::json::MakeObject("id", alert.id, "message",
                                               alert.description));
  }

  return formats::json::MakeObject("alerts", builder.ExtractValue());
}

}  // namespace alerts

USERVER_NAMESPACE_END
