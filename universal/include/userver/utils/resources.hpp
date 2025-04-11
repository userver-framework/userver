#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils {

void RegisterResource(std::string_view name, std::string_view value);

std::string FindResource(std::string_view name);

}  // namespace utils

USERVER_NAMESPACE_END
