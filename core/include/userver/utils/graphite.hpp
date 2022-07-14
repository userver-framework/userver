#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::graphite {

std::string EscapeName(const std::string& s);

}  // namespace utils::graphite

USERVER_NAMESPACE_END
