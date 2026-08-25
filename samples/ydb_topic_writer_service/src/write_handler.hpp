#pragma once

#include <string_view>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/ydb/topic_writer.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace samples::ydb_topic_writer {

/// [topic writer handler decl]
class WriteHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName{"handler-write"};

    WriteHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& context)
        const override;

private:
    ydb::TopicWriterManager& writer_manager_;
    ydb::TopicWriter& writer_;
};
/// [topic writer handler decl]

}  // namespace samples::ydb_topic_writer
