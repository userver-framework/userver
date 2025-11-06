#pragma once

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

logging::Level ConvertRdKafkaLogLevelToLoggingLevel(int log_level) noexcept;

}

USERVER_NAMESPACE_END
