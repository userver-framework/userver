#pragma once

#include <string>

#include <userver/logging/format.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

const std::string& GetSpdlogPattern(Format format);

}  // namespace logging

USERVER_NAMESPACE_END
