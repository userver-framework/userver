#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <userver/storages/odbc/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::tracing {

struct QuerySpanTags final {
    std::optional<std::string_view> statement_name;
    std::optional<std::string_view> statement;
};

std::string MakeQuerySpanName(std::string_view statement);

QuerySpanTags MakeQuerySpanTags(const Query& query) noexcept;

}  // namespace storages::odbc::detail::tracing

USERVER_NAMESPACE_END
