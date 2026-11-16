#pragma once

#include <test/tagged/api/handlers/usecredential/requests.hpp>
#include <test/tagged/api/handlers/usecredential/responses.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/request/request_context.hpp>

namespace test::tagged::api::handlers::usecredential {

struct HandlerTag;

class View final {
 public:
  using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;
  using RequestContext = USERVER_NAMESPACE::server::request::RequestContext;

  static Response Handle(Request&& request, Deps&& deps, RequestContext& context);
};

}  // namespace test::tagged::api::handlers::usecredential
