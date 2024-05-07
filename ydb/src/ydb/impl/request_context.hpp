#pragma once

#include <ydb-cpp-sdk/client/retry/retry.h>

#include <exception>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <userver/ydb/query.hpp>
#include <userver/ydb/table.hpp>

#include <ydb/impl/operation_settings.hpp>
#include <ydb/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

struct RequestContext final {
  RequestContext(TableClient& client, const Query& query,
                 OperationSettings& settings,
                 IsStreaming is_streaming = IsStreaming{false},
                 tracing::Span* custom_parent_span = nullptr,
                 const utils::impl::SourceLocation& location =
                     utils::impl::SourceLocation::Current());

  ~RequestContext();

  TableClient& table_client;
  OperationSettings& settings;
  const int initial_uncaught_exceptions;
  StatsScope stats_scope;
  dynamic_config::Snapshot config_snapshot;
  tracing::Span span;
  engine::Deadline deadline;
};

}  // namespace ydb::impl

USERVER_NAMESPACE_END
