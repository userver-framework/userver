#include <ydb/impl/request_context.hpp>

#include <exception>

#include <userver/formats/json/inline.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <dynamic_config/variables/YDB_DEADLINE_PROPAGATION_VERSION.hpp>
#include <dynamic_config/variables/YDB_QUERIES_COMMAND_CONTROL.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

template <typename Settings>
void AddSpecificTags(tracing::Span& span, Settings& settings);

template <>
void AddSpecificTags<OperationSettings>(tracing::Span& span, OperationSettings& settings) {
    UASSERT(settings.retries.has_value());
    span.AddTag("max_retries", *settings.retries);
    span.AddTag("get_session_timeout_ms", settings.get_session_timeout_ms.count());
    span.AddTag("client_timeout_ms", settings.client_timeout_ms.count());
    settings.trace_id = span.GetTraceId();
}

template <>
void AddSpecificTags<RequestSettings>(tracing::Span& span, RequestSettings& settings) {
    span.AddTag("timeout_ms", settings.timeout_ms.count());
    settings.trace_id = span.GetTraceId();
}

template <>
void AddSpecificTags<RetryTxSettings>(tracing::Span& span, RetryTxSettings& settings) {
    span.AddTag("timeout_ms", settings.timeout_ms.count());
    span.AddTag("retries", settings.retries);
    span.AddTag("is_idempotent", settings.is_idempotent);
}

template <typename Settings>
tracing::Span MakeSpan(
    const Query& query,
    Settings& settings,
    tracing::Span* custom_parent_span,
    utils::impl::SourceLocation location
) {
    auto span =
        custom_parent_span
            ? custom_parent_span->CreateChild("ydb_query", location)
            : tracing::Span("ydb_query", location);

    const auto optional_name_view = query.GetOptionalNameView();
    switch (query.GetLogMode()) {
        case Query::LogMode::kFull:
            if (optional_name_view) {
                span.AddTag("query_name", std::string{*optional_name_view});
            } else {
                span.AddTag("yql_query", std::string{query.GetStatementView()});
            }
            break;
        case Query::LogMode::kNameOnly:
            if (optional_name_view) {
                span.AddTag("query_name", std::string{*optional_name_view});
            }
            break;
    }

    AddSpecificTags<Settings>(span, settings);

    if (optional_name_view) {
        try {
            TESTPOINT("sql_statement", formats::json::MakeObject("name", *optional_name_view));
        } catch (const std::exception& e) {
            LOG_WARNING() << e;
        }
    }

    return span;
}

template <typename Settings>
void PrepareSettings(
    const Query& query,
    const dynamic_config::Snapshot& config_snapshot,
    Settings& os,
    impl::IsStreaming is_streaming,
    const OperationSettings& default_settings
);

template <>
void PrepareSettings<OperationSettings>(
    const Query& query,
    const dynamic_config::Snapshot& config_snapshot,
    OperationSettings& os,
    impl::IsStreaming is_streaming,
    const OperationSettings& default_settings
) {
    // Priority of the OperationSettings choosing. From low to high:
    // 0. Driver's defaults
    // 1. Static config
    // 2. OperationSettings passed in code
    // 3. Dynamic config

    if (!os.retries.has_value()) {
        os.retries = default_settings.retries;
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
        os.tx_mode = default_settings.tx_mode;
    }

    const auto& cc_map = config_snapshot[::dynamic_config::YDB_QUERIES_COMMAND_CONTROL];

    if (!query.GetOptionalNameView()) {
        return;
    }
    auto it = cc_map.extra.find(std::string{*query.GetOptionalNameView()});  // TODO: avoid tmp string construction
    if (it == cc_map.extra.end()) {
        return;
    }

    auto& cc = it->second;

    if (cc.attempts.has_value()) {
        UASSERT(*cc.attempts > 0);
        os.retries = *cc.attempts - 1;
    }
    if (cc.client_timeout_ms) {
        os.client_timeout_ms = cc.client_timeout_ms.value();
    }
    if (cc.get_session_timeout_ms) {
        os.get_session_timeout_ms = cc.get_session_timeout_ms.value();
    }
}

template <>
void PrepareSettings<RequestSettings>(
    [[maybe_unused]] const Query& query,
    [[maybe_unused]] const dynamic_config::Snapshot& config_snapshot,
    [[maybe_unused]] RequestSettings& os,
    [[maybe_unused]] impl::IsStreaming is_streaming,
    [[maybe_unused]] const OperationSettings& default_settings
) {
    // TODO: to think about default settings for RequestSettings
}

template <>
void PrepareSettings<RetryTxSettings>(
    [[maybe_unused]] const Query& query,
    [[maybe_unused]] const dynamic_config::Snapshot& config_snapshot,
    [[maybe_unused]] RetryTxSettings& os,
    [[maybe_unused]] impl::IsStreaming is_streaming,
    [[maybe_unused]] const OperationSettings& default_settings
) {
    // TODO: to think about default settings for RetryTxSettings
}

}  // namespace

engine::Deadline GetDeadline(tracing::Span& span, const dynamic_config::Snapshot& config_snapshot) {
    if (config_snapshot[::dynamic_config::YDB_DEADLINE_PROPAGATION_VERSION] !=
        impl::kDeadlinePropagationExperimentVersion)
    {
        LOG_DEBUG()
            << "Wrong DP experiment version, config="
            << config_snapshot[::dynamic_config::YDB_DEADLINE_PROPAGATION_VERSION]
            << ", experiment=" << impl::kDeadlinePropagationExperimentVersion;
        return {};
    }

    if (!utils::impl::kYdbDeadlinePropagationExperiment.IsEnabled()) {
        LOG_DEBUG() << "Deadline propagation is disabled via experiment";
        return {};
    }
    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();

    if (inherited_deadline.IsReachable()) {
        span.AddTag(
            "deadline_timeout_ms",
            std::chrono::duration_cast<std::chrono::milliseconds>(inherited_deadline.TimeLeft()).count()
        );
    }

    return inherited_deadline;
}

template <typename Settings>
RequestContext<Settings>::RequestContext(
    TableClient& l_table_client,
    const Query& query,
    Settings&& settings,
    IsStreaming is_streaming,
    tracing::Span* custom_parent_span,
    engine::Deadline parent_deadline,
    const utils::impl::SourceLocation& location
)
    : table_client(l_table_client),
      settings(std::move(settings)),
      initial_uncaught_exceptions(std::uncaught_exceptions()),
      stats_scope(*table_client.stats_, query),
      config_snapshot(table_client.config_source_.GetSnapshot()),
      // Note: comma operator is used to insert code between initializations.
      span((
          PrepareSettings<
              Settings>(query, config_snapshot, this->settings, is_streaming, table_client.default_settings_),
          MakeSpan<Settings>(query, this->settings, custom_parent_span, location)
      )),
      deadline(std::min(GetDeadline(span, config_snapshot), parent_deadline))
{}

template <typename Settings>
void RequestContext<Settings>::HandleError(const NYdb::TStatus& status) {
    if (engine::current_task::ShouldCancel()) {
        return;
    }
    UASSERT(!status.IsSuccess());
    // To protect against double handling of error in the 'HandleError` and in the
    // destructor we have to set the flag
    is_error = true;
    span.AddTag(tracing::kErrorFlag, true);
    if (status.IsTransportError()) {
        stats_scope.OnTransportError();
    } else {
        stats_scope.OnError();
    }
}

template <typename Settings>
RequestContext<Settings>::~RequestContext() {
    if (engine::current_task::ShouldCancel() && !is_error) {
        stats_scope.OnCancelled();
        span.AddTag("cancelled", true);
    }
}

template class RequestContext<OperationSettings>;
template class RequestContext<RequestSettings>;
template class RequestContext<RetryTxSettings>;

}  // namespace ydb::impl

USERVER_NAMESPACE_END
