#include <userver/server/http/http_response_body_stream.hpp>

#include <server/http/http_response_impl.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

struct ResponseBodyStream::BodyProducer {
    HttpResponseImpl::Producer producer;
};

namespace {

auto TransferToStream(HttpResponse& response) {
    auto& http_response = GetHttpResponseImpl(response);
    http_response.SetStreamBody();
    return http_response.GetBodyProducer();
}

}  // namespace

ResponseBodyStream::ResponseBodyStream(HttpResponse& http_response)
    : body_producer_(BodyProducer{TransferToStream(http_response)}),
      http_response_(http_response)
{}

ResponseBodyStream::~ResponseBodyStream() {
    const auto& http_response = GetHttpResponseImpl(http_response_);
    if (http_response.GetStreamId().has_value()) {
        UASSERT(body_producer_->producer.index() == 2);
        std::get<impl::Http2StreamEventProducer>(body_producer_->producer).CloseStream(*http_response.GetStreamId());
    }
}

void ResponseBodyStream::PushBodyChunk(std::string&& chunk, engine::Deadline deadline) {
    UASSERT_MSG(headers_ended_, "SetEndOfHeaders() was not called before PushBodyChunk()");
    UASSERT_MSG(http_response_.GetData().empty(), "PushBodyChunk() was called after SetBody()");

    if (headers_ended_ && !headers_end_sent_) {
        http_response_.SetHeadersEnd();
        headers_end_sent_ = true;
    }
    std::visit(
        utils::Overloaded{
            [&chunk, &deadline](concurrent::StringStreamQueue::Producer& queue_producer) mutable {
                const bool success = queue_producer.Push(std::move(chunk), deadline);
                UASSERT(success);
            },
            [this, &chunk, &deadline](impl::Http2StreamEventProducer& queue_producer) mutable {
                const auto& http_response = GetHttpResponseImpl(http_response_);
                UASSERT(http_response.GetStreamId().has_value());
                queue_producer.PushEvent({*http_response.GetStreamId(), std::move(chunk)}, deadline);
            },
            [](std::monostate) { UINVARIANT(false, "unreachable"); }
        },
        body_producer_->producer
    );
}

void ResponseBodyStream::SetBody(std::string&& body) {
    UASSERT_MSG(!headers_end_sent_, "SetBody() was called after PushBodyChunk()");
    http_response_.SetData(std::move(body));
}

void ResponseBodyStream::SetHeader(const std::string& name, const std::string& value) {
    http_response_.SetHeader(name, value);
}

void ResponseBodyStream::SetHeader(std::string_view name, const std::string& value) {
    http_response_.SetHeader(name, value);
}

void ResponseBodyStream::SetEndOfHeaders() { headers_ended_ = true; }

void ResponseBodyStream::SetStatusCode(int status_code) {
    http_response_.SetStatus(static_cast<server::http::HttpStatus>(status_code));
}

void ResponseBodyStream::SetStatusCode(HttpStatus status) { http_response_.SetStatus(status); }

}  // namespace server::http

USERVER_NAMESPACE_END
