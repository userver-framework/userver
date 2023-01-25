#pragma once

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

std::string ToLowerCase(const std::string& header_name);

bool IsLowerCase(std::string_view header_name);

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
