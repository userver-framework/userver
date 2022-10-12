#pragma once

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::http {

std::string_view GetCachedHttpDate();

std::string GetHttpDate();

}  // namespace server::http

USERVER_NAMESPACE_END
