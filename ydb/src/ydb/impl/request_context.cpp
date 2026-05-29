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

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

void AddSpecificTags(tracing::Span& span, OperationSettings& settings) {
    UASSERT(settings.retries.has_value());
    span.AddTag("max_retries", settings.retries.value());
    span.AddTag("get_session_timeout_ms", settings.get_session_timeout_ms.count());
    span.AddTag("client_timeout_ms", settings.client_timeout_ms.count());
    span.AddTag("is_idempotent", settings.is_idempotent);
    settings.trace_id = span.GetTraceId();
}

void AddSpecificTags(tracing::Span& span, RequestSettings& settings) {
    if (settings.client_timeout_ms.has_value() && settings.client_timeout_ms.value() != 0ms) {
        span.AddTag("client_timeout_ms", settings.client_timeout_ms.value().count());
    } else {
        span.AddTag("client_timeout_ms", "unlimited");
    }
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

    AddSpecificTags(span, settings);

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
bool IsTimeoutSet(const Settings& settings) {
    if constexpr (std::is_same_v<Settings, OperationSettings>) {
        return settings.client_timeout_ms != std::chrono::milliseconds::zero();
    } else {
        return settings.client_timeout_ms.has_value();
    }
}

// Priority of the Settings choosing. From low to high:
// 0. Driver's defaults
// 1. Static config
// 2. Settings passed in code
// 3. Dynamic config
template <typename Settings>
void PrepareSettings(
    const Query& query,
    const dynamic_config::Snapshot& config_snapshot,
    Settings& os,
    impl::IsStreaming is_streaming,
    const OperationSettings& default_settings
) {
    // For streaming operations, client timeout is applied to the entire
    // streaming RPC. Meanwhile, streaming RPCs can be expected to take
    // an unbounded amount of time. YDB gRPC machinery automatically checks
    // that the server has not died, otherwise we'll get an exception.
    //
    // Timeouts specified in code, as well as in dynamic config, still apply.
    // NOLINTNEXTLINE(bugprone-non-zero-enum-to-bool-conversion)
    if (!static_cast<bool>(is_streaming)) {
        if (!IsTimeoutSet(os)) {
            os.client_timeout_ms = default_settings.client_timeout_ms;
        }
    }

    if constexpr (std::is_same_v<Settings, OperationSettings>) {
        if (!os.tx_mode.has_value()) {
            os.tx_mode = default_settings.tx_mode;
        }

        if (!os.retries.has_value()) {
            os.retries = default_settings.retries;
        }

        if (os.get_session_timeout_ms == std::chrono::milliseconds::zero()) {
            os.get_session_timeout_ms = default_settings.get_session_timeout_ms;
        }
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

    if (cc.client_timeout_ms.has_value()) {
        os.client_timeout_ms = cc.client_timeout_ms.value();
    }

    if constexpr (std::is_same_v<Settings, OperationSettings>) {
        if (cc.attempts.has_value()) {
            UASSERT(*cc.attempts > 0);
            os.retries = *cc.attempts - 1;
        }
        if (cc.get_session_timeout_ms.has_value()) {
            os.get_session_timeout_ms = cc.get_session_timeout_ms.value();
        }
    }
}

}  // namespace

/// Priority of the RetryTxSettings choosing. From low to high:
/// 0. Driver's defaults
/// 1. Static config
/// 2. RetryTxSettings passed in code
void PrepareSettings(RetryTxSettings& rs, const OperationSettings& default_settings) {
    if (!rs.retries.has_value()) {
        rs.retries = default_settings.retries;
    }

    if (!rs.tx_mode.has_value()) {
        rs.tx_mode = default_settings.tx_mode;
    }
}

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

}  // namespace ydb::impl

USERVER_NAMESPACE_END
