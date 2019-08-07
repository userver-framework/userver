#pragma once

#include <logging/spdlog.hpp>

#include <logging/logger.hpp>

namespace logging {
namespace impl {

void SinkMessage(const logging::LoggerPtr&, spdlog::details::log_msg&);

}
}  // namespace logging
