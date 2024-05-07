#include <ydb/impl/request_context.hpp>

#include <exception>

#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

RequestContext::RequestContext(TableClient& table_client_, const Query& query,
                               OperationSettings& settings,
                               IsStreaming is_streaming,
                               tracing::Span* custom_parent_span,
                               const utils::impl::SourceLocation& location)
    : table_client(table_client_),
      settings(settings),
      initial_uncaught_exceptions(std::uncaught_exceptions()),
      stats_scope(*table_client.stats_, query),
      config_snapshot(table_client.config_source_.GetSnapshot()),
      // Note: comma operator is used to insert code between initializations.
      span((table_client.PrepareSettings(query, config_snapshot, settings,
                                         is_streaming),
            table_client.MakeSpan(query, settings, custom_parent_span,
                                  location))),
      deadline(table_client.GetDeadline(span, config_snapshot)) {}

RequestContext::~RequestContext() {
  if (std::uncaught_exceptions() > initial_uncaught_exceptions) {
    stats_scope.OnError();
    span.AddTag(tracing::kErrorFlag, true);
  }
  if (engine::current_task::ShouldCancel()) {
    stats_scope.OnCancelled();
    span.AddTag("cancelled", true);
  }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
