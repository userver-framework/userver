#pragma once

#include <logging/spdlog.hpp>

#include <logging/logger.hpp>

namespace logging::impl {

void SinkMessage(const logging::LoggerPtr&, spdlog::details::log_msg&);

}  // namespace logging::impl
