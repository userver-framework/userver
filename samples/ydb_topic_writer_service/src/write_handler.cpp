#include "write_handler.hpp"

#include <userver/components/component_context.hpp>
#include <userver/http/content_type.hpp>
#include <userver/ydb/topic_writer_component.hpp>

namespace samples::ydb_topic_writer {

namespace {

constexpr std::string_view kWriterName = "messages";

}  // namespace

/// [topic writer handler ctor]
WriteHandler::WriteHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      writer_manager_{context.FindComponent<ydb::TopicWriterComponent>().GetTopicWriterManager()},
      writer_(writer_manager_.GetTopicWriter(kWriterName))
{}
/// [topic writer handler ctor]

/// [topic writer HandleRequest]
std::string WriteHandler::HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);

    const auto result = writer_.WriteMessage(request.RequestBody(), {});
    switch (result.GetStatus()) {
        case ydb::TopicWriteStatus::kOk:
            return "Message was written to topic\n";
        case ydb::TopicWriteStatus::kResourceExhausted:
            request.SetResponseStatus(server::http::HttpStatus::kTooManyRequests);
            return "Topic writer queue is full\n";
        case ydb::TopicWriteStatus::kFail:
        case ydb::TopicWriteStatus::kNotReady:
        case ydb::TopicWriteStatus::kMaxCount:
            request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
            return "Failed to write message to topic\n";
    }

    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
    return "Unexpected topic writer status\n";
}
/// [topic writer HandleRequest]

}  // namespace samples::ydb_topic_writer
