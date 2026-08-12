#pragma once

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::tracing {

std::string MakeQuerySpanName(std::string_view statement);

}  // namespace storages::odbc::detail::tracing

USERVER_NAMESPACE_END
