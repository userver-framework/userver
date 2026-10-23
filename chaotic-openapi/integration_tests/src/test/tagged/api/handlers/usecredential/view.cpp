#include "view.hpp"

#include <userver/formats/json/value_builder.hpp>

namespace test::tagged::api::handlers::usecredential {

Response View::Handle(Request&& request, Deps&& /*deps*/, RequestContext& /*context*/) {
  // A body without a schema is any JSON value, passed through as is
  USERVER_NAMESPACE::formats::json::ValueBuilder payload{request.body};
  payload["name"] = request.name;
  return Response200{payload.ExtractValue()};
}

}  // namespace test::tagged::api::handlers::usecredential
