#include "logcapture.hpp"

#include <userver/testsuite/testpoint.hpp>

namespace tests::handlers {

formats::json::Value LogCapture::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
    LOG_INFO() << "Message to capture";
    return {};
}

}  // namespace tests::handlers
