#include <userver/server/http/http_response_body_stream.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

ResponseBodyStream::ResponseBodyStream(
    server::http::HttpResponse::Producer&& queue_producer,
    server::http::HttpResponse& http_response
)
    : queue_producer_(std::move(queue_producer)), http_response_(http_response) {}

ResponseBodyStream::~ResponseBodyStream() {
    if (http_response_.GetStreamId().has_value()) {
        UASSERT(queue_producer_.index() == 2);
        std::get<impl::Http2StreamEventProducer>(queue_producer_).CloseStream(*http_response_.GetStreamId());
    }
}

void ResponseBodyStream::PushBodyChunk(std::string&& chunk, engine::Deadline deadline) {
    UASSERT_MSG(headers_ended_, "SetEndOfHeaders() was not called before PushBodyChunk()");
    std::visit(
        utils::Overloaded{
            [&chunk, &deadline](HttpResponse::Queue::Producer& queue_producer) mutable {
                const bool success = queue_producer.Push(std::move(chunk), deadline);
                UASSERT(success);
            },
            [this, &chunk, &deadline](impl::Http2StreamEventProducer& queue_producer) mutable {
                UASSERT(http_response_.GetStreamId().has_value());
                queue_producer.PushEvent({*http_response_.GetStreamId(), std::move(chunk)}, deadline);
            },
            [](std::monostate) { UINVARIANT(false, "unreachable"); }},
        queue_producer_
    );
}

void ResponseBodyStream::SetHeader(const std::string& name, const std::string& value) {
    http_response_.SetHeader(name, value);
}

void ResponseBodyStream::SetHeader(std::string_view name, const std::string& value) {
    http_response_.SetHeader(name, value);
}

void ResponseBodyStream::SetEndOfHeaders() {
    headers_ended_ = true;
    http_response_.SetHeadersEnd();
}

void ResponseBodyStream::SetStatusCode(int status_code) {
    http_response_.SetStatus(static_cast<server::http::HttpStatus>(status_code));
}

void ResponseBodyStream::SetStatusCode(HttpStatus status) { http_response_.SetStatus(status); }

}  // namespace server::http

USERVER_NAMESPACE_END
