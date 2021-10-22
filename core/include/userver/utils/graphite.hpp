#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils {
namespace graphite {
std::string EscapeName(const std::string& s);
}  // namespace graphite
}  // namespace utils

USERVER_NAMESPACE_END
