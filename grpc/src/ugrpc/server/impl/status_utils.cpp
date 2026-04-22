#include <userver/ugrpc/server/impl/status_utils.hpp>

#include <boost/algorithm/string.hpp>

#include <userver/ugrpc/impl/status_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// ASCII SP (0x20) or HTAB (0x09) - forbidden at field value boundaries (RFC 9113 8.2.1)
// https://datatracker.ietf.org/doc/html/rfc9113#section-8.2.1-3.4
// But 0x09 is percent-encoded before transfer by grpc, so we need to trim only 0x20
// https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md#responses
constexpr char kForbiddenBoundaryHttp2Whitespace = 0x20;

}  // namespace

namespace ugrpc::server::impl {

void TrimStatusErrorMessage(grpc::Status& status) {
    const auto& error_message = status.error_message();
    if (!error_message.empty() &&
        ((error_message.front() == kForbiddenBoundaryHttp2Whitespace) ||
         (error_message.back() == kForbiddenBoundaryHttp2Whitespace)))
    {
        status = grpc::Status{
            status.error_code(),
            boost::trim_copy_if(error_message, [](auto c) { return c == kForbiddenBoundaryHttp2Whitespace; }),
            status.error_details(),
        };
    }
}

void NormalizeStatus(grpc::Status& status) {
    TrimStatusErrorMessage(status);
    ugrpc::impl::ClampStatusCodeToValidRange(status);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
