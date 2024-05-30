#include <ydb/impl/request_context.hpp>

#include <exception>

#include <userver/formats/json/inline.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

tracing::Span MakeSpan(const Query& query, OperationSettings& settings,
                       tracing::Span* custom_parent_span,
                       utils::impl::SourceLocation location) {
  auto span = custom_parent_span
                  ? custom_parent_span->CreateChild("ydb_query")
                  : tracing::Span("ydb_query", tracing::ReferenceType::kChild,
                                  logging::Level::kInfo, location);

  settings.trace_id = span.GetTraceId();

  if (query.GetName()) {
    span.AddTag("query_name", std::string{*query.GetName()});
  } else {
    span.AddTag("yql_query", query.Statement());
  }
  UASSERT(settings.retries.has_value());
  span.AddTag("max_retries", *settings.retries);
  span.AddTag("get_session_timeout_ms",
              settings.get_session_timeout_ms.count());
  span.AddTag("operation_timeout_ms", settings.operation_timeout_ms.count());
  span.AddTag("cancel_after_ms", settings.cancel_after_ms.count());
  span.AddTag("client_timeout_ms", settings.client_timeout_ms.count());

  if (query.GetName()) {
    try {
      TESTPOINT("sql_statement", formats::json::MakeObject(
                                     "name", query.GetName()->GetUnderlying()));
    } catch (const std::exception& e) {
      LOG_WARNING() << e;
    }
  }

  return span;
}

void PrepareSettings(const Query& query,
                     const dynamic_config::Snapshot& config_snapshot,
                     OperationSettings& os, impl::IsStreaming is_streaming,
                     const OperationSettings& default_settings) {
  // Priority of the OperationSettings choosing. From low to high:
  // 0. Driver's defaults
  // 1. Static config
  // 2. OperationSettings passed in code
  // 3. Dynamic config

  if (!os.retries.has_value()) {
    os.retries = default_settings.retries.value();
  }
  if (os.operation_timeout_ms == std::chrono::milliseconds::zero()) {
    os.operation_timeout_ms = default_settings.operation_timeout_ms;
  }
  if (os.cancel_after_ms == std::chrono::milliseconds::zero()) {
    os.cancel_after_ms = default_settings.cancel_after_ms;
  }
  // For streaming operations, client timeout is applied to the entire
  // streaming RPC. Meanwhile, streaming RPCs can be expected to take
  // an unbounded amount of time. YDB gRPC machinery automatically checks
  // that the server has not died, otherwise we'll get an exception.
  //
  // Timeouts specified in code, as well as in dynamic config, still apply.
  // NOLINTNEXTLINE(bugprone-non-zero-enum-to-bool-conversion)
  if (!static_cast<bool>(is_streaming)) {
    if (os.client_timeout_ms == std::chrono::milliseconds::zero()) {
      os.client_timeout_ms = default_settings.client_timeout_ms;
    }
  }
  if (os.get_session_timeout_ms == std::chrono::milliseconds::zero()) {
    os.get_session_timeout_ms = default_settings.get_session_timeout_ms;
  }
  if (!os.tx_mode) {
    os.tx_mode = default_settings.tx_mode.value();
  }

  const auto& cc_map = config_snapshot[impl::kQueryCommandControl];

  if (!query.GetName()) return;
  auto it = cc_map.find(query.GetName()->GetUnderlying());
  if (it == cc_map.end()) return;

  auto& cc = it->second;

  if (cc.attempts.has_value()) {
    UASSERT(*cc.attempts > 0);
    os.retries = *cc.attempts - 1;
  }
  if (cc.operation_timeout_ms)
    os.operation_timeout_ms = cc.operation_timeout_ms.value();
  if (cc.cancel_after_ms) os.cancel_after_ms = cc.cancel_after_ms.value();
  if (cc.client_timeout_ms) os.client_timeout_ms = cc.client_timeout_ms.value();
  if (cc.get_session_timeout_ms)
    os.get_session_timeout_ms = cc.get_session_timeout_ms.value();
}

utils::impl::UserverExperiment kYdbDeadlinePropagationExperiment(
    "ydb-deadline-propagation");

engine::Deadline GetDeadline(tracing::Span& span,
                             const dynamic_config::Snapshot& config_snapshot) {
  if (config_snapshot[impl::kDeadlinePropagationVersion] !=
      impl::kDeadlinePropagationExperimentVersion) {
    LOG_DEBUG() << "Wrong DP experiment version, config="
                << config_snapshot[impl::kDeadlinePropagationVersion]
                << ", experiment="
                << impl::kDeadlinePropagationExperimentVersion;
    return {};
  }

  if (!kYdbDeadlinePropagationExperiment.IsEnabled()) {
    LOG_DEBUG() << "Deadline propagation is disabled via experiment";
    return {};
  }
  const auto inherited_deadline = server::request::GetTaskInheritedDeadline();

  if (inherited_deadline.IsReachable()) {
    span.AddTag("deadline_timeout_ms",
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    inherited_deadline.TimeLeft())
                    .count());
  }

  return inherited_deadline;
}

}  // namespace

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
      span((PrepareSettings(query, config_snapshot, settings, is_streaming,
                            table_client.default_settings_),
            MakeSpan(query, settings, custom_parent_span, location))),
      deadline(GetDeadline(span, config_snapshot)) {}

void RequestContext::HandleError(const NYdb::TStatus& status) {
  if (engine::current_task::ShouldCancel()) {
    return;
  }
  UASSERT(!status.IsSuccess());
  // To protect against double handling of error in the 'HandleError` and in the
  // destructor we have to set the flag
  is_error_ = true;
  span.AddTag(tracing::kErrorFlag, true);
  if (status.IsTransportError()) {
    stats_scope.OnTransportError();
  } else {
    stats_scope.OnError();
  }
}

RequestContext::~RequestContext() {
  if (engine::current_task::ShouldCancel() && !is_error_) {
    stats_scope.OnCancelled();
    span.AddTag("cancelled", true);
  }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
