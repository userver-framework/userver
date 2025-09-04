#pragma once

/// @file userver/storages/clickhouse/query.hpp
/// @brief @copybrief storages::clickhouse::Query

#include <string>

#include <fmt/format.h>

#include <userver/storages/query.hpp>
#include <userver/utils/fmt_compat.hpp>

#include <userver/storages/clickhouse/io/impl/escape.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

/// @copydoc storages::Query
using storages::Query;

namespace impl {

template <typename... Args>
Query WithArgs(const Query& query, const Args&... args) {
    // we should throw on params count mismatch
    // TODO : https://st.yandex-team.ru/TAXICOMMON-5066
    return Query{
        fmt::format(fmt::runtime(query.GetStatementView()), io::impl::Escape(args)...), query.GetOptionalName()};
}

}  // namespace impl

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
