#include "now.hpp"

#include <userver/utils/datetime.hpp>

namespace tests::handlers {

formats::json::Value Now::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context) const {
  formats::json::ValueBuilder result;
  result["now"] = utils::datetime::Now();
  return result.ExtractValue();
}

}  // namespace tests::handlers
