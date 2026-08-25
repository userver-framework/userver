#include <storages/mongo/cdriver/request_helpers.hpp>

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

#include <dynamic_config/variables/MONGO_DEFAULT_MAX_TIME_MS.hpp>
#include <dynamic_config/variables/USERVER_DEADLINE_PROPAGATION_ENABLED.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

const std::string kCancelledByDeadlineTag = "cancelled_by_deadline";
const std::string kCancelledTag = "cancelled";
const std::string kMaxTimeMsTag = "max_time_ms";

std::optional<std::string_view> GetCurrentSpanLink() {
    auto* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        return span->GetLink();
    }
    return std::nullopt;
}

std::optional<std::chrono::milliseconds> GetDeadlineTimeLeft(const dynamic_config::Snapshot& config) {
    if (!config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_ENABLED]) {
        return std::nullopt;
    }

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (!inherited_deadline.IsReachable()) {
        return std::nullopt;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(inherited_deadline.TimeLeftApprox());
}

void SetLinkComment(formats::bson::impl::BsonBuilder& builder, bool& has_comment_option) {
    auto link = GetCurrentSpanLink();
    if (link) {
        operations::AppendComment(builder, has_comment_option, options::Comment(utils::StrCat("link=", *link)));
    }
}

std::optional<std::chrono::milliseconds> GetTimeoutOrThrow(
    const dynamic_config::Snapshot& dynamic_config,
    stats::OperationStatisticsItem& stats,
    tracing::Span& span
) {
    const auto time_left = GetDeadlineTimeLeft(dynamic_config);
    if (time_left && time_left <= std::chrono::seconds{0}) {
        stats.Account(stats::ErrorType::kCancelled);
        span.AddTag(kCancelledByDeadlineTag, true);
        throw CancelledException(CancelledException::ByDeadlinePropagation{});
    }
    return time_left;
}

void TagTimeout(tracing::Span& span, const std::optional<std::chrono::milliseconds>& timeout_ms) {
    if (timeout_ms) {
        span.AddTag(tracing::kTimeoutMs, timeout_ms->count());
    }
}

}  // namespace

CDriverPoolImpl& GetCDriverPool(const PoolImplPtr& pool_impl) {
    auto* pool = dynamic_cast<CDriverPoolImpl*>(pool_impl.get());
    UASSERT(pool);
    return *pool;
}

CDriverPoolImpl::BoundClientPtr AcquireClient(CDriverPoolImpl& pool, stats::OperationStatisticsItem& stats) {
    try {
        return pool.Acquire();
    } catch (const CancelledException& ex) {
        stats.Account(stats::ErrorType::kCancelled);
        auto& span = tracing::Span::CurrentSpan();
        if (ex.IsByDeadlinePropagation()) {
            span.AddTag(kCancelledByDeadlineTag, true);
        } else {
            span.AddTag(kCancelledTag, true);
        }
        throw;
    } catch (const PoolOverloadException& /*ex*/) {
        stats.Account(stats::ErrorType::kPoolOverload);
        throw;
    }
}

RequestContextBase MakeRequestContextBase(
    tracing::Span span,
    std::shared_ptr<stats::OperationStatisticsItem> stats,
    dynamic_config::Snapshot dynamic_config,
    utils::function_ref<CDriverPoolImpl::BoundClientPtr(stats::OperationStatisticsItem&)> get_client
) {
    auto timeout_ms = GetTimeoutOrThrow(dynamic_config, *stats, span);
    TagTimeout(span, timeout_ms);

    auto client = get_client(*stats);

    timeout_ms = timeout_ms ? GetTimeoutOrThrow(dynamic_config, *stats, span) : timeout_ms;
    TagTimeout(span, timeout_ms);

    return RequestContextBase{
        .stats = std::move(stats),
        .dynamic_config = std::move(dynamic_config),
        .client = std::move(client),
        .span = std::move(span),
        .inherited_deadline = timeout_ms,
    };
}

std::chrono::milliseconds ComputeAdjustedMaxServerTime(
    std::chrono::milliseconds user_max_server_time,
    const RequestContextBase& context
) {
    auto max_server_time = user_max_server_time;
    try {
        operations::VerifyMaxServerTime(max_server_time);
    } catch (const InvalidQueryArgumentException& /*ex*/) {
        context.stats->Account(stats::ErrorType::kBadQueryArgument);
        throw;
    }

    if (max_server_time == operations::kNoMaxServerTime) {
        max_server_time = context.dynamic_config[::dynamic_config::MONGO_DEFAULT_MAX_TIME_MS];
    }

    if (auto inherited_deadline = context.inherited_deadline) {
        operations::VerifyMaxServerTime(*inherited_deadline);
        if (max_server_time == operations::kNoMaxServerTime || *inherited_deadline < max_server_time) {
            max_server_time = *inherited_deadline;
        }
    }

    if (max_server_time != operations::kNoMaxServerTime) {
        tracing::Span::CurrentSpan().AddTag(kMaxTimeMsTag, max_server_time.count());
    }

    return max_server_time;
}

void SetMaxServerTime(
    std::optional<formats::bson::impl::BsonBuilder>& builder,
    std::chrono::milliseconds max_server_time,
    const RequestContextBase& context
) {
    max_server_time = ComputeAdjustedMaxServerTime(max_server_time, context);
    if (max_server_time == operations::kNoMaxServerTime) {
        return;
    }

    constexpr std::string_view kOptionName = "maxTimeMS";
    EnsureBuilder(builder).Append(kOptionName, max_server_time.count());
}

void PrepareCursorOptions(
    std::optional<formats::bson::impl::BsonBuilder>& options,
    std::chrono::milliseconds max_server_time,
    bool has_comment_option,
    const RequestContextBase& context
) {
    SetMaxServerTime(options, max_server_time, context);
    if (!has_comment_option) {
        SetLinkComment(EnsureBuilder(options), has_comment_option);
    }
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
