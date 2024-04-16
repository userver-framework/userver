#include <server/middlewares/baggage.hpp>

#include <server/request/internal_request_context.hpp>

#include <userver/baggage/baggage.hpp>
#include <userver/baggage/baggage_settings.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

void SetUpBaggage(const http::HttpRequest& http_request,
                  const dynamic_config::Snapshot& config_snapshot) {
  if (config_snapshot[baggage::kBaggageEnabled]) {
    auto baggage_header =
        http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXBaggage);
    if (!baggage_header.empty()) {
      LOG_DEBUG() << "Got baggage header: " << baggage_header;
      const auto& baggage_settings = config_snapshot[baggage::kBaggageSettings];
      auto baggage = baggage::TryMakeBaggage(std::move(baggage_header),
                                             baggage_settings.allowed_keys);
      if (baggage) {
        baggage::kInheritedBaggage.Set(std::move(*baggage));
      }
    }
  }
}

Baggage::Baggage(const handlers::HttpHandlerBase&) {}

void Baggage::HandleRequest(http::HttpRequest& request,
                            request::RequestContext& context) const {
  SetUpBaggage(request, context.GetInternalContext().GetConfigSnapshot());

  Next(request, context);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
