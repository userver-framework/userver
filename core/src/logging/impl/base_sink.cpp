#include "base_sink.hpp"

#include <spdlog/pattern_formatter.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
BaseSink::BaseSink()
    : formatter_{std::make_unique<spdlog::pattern_formatter>()} {}

void BaseSink::log(const spdlog::details::log_msg& msg) {
  std::lock_guard lock{mutex_};

  spdlog::memory_buf_t formatted;
  formatter_->format(msg, formatted);

  Write({formatted.data(), formatted.size()});
}

void BaseSink::set_pattern(const std::string& pattern) {
  set_formatter(std::make_unique<spdlog::pattern_formatter>(pattern));
}

void BaseSink::set_formatter(
    std::unique_ptr<spdlog::formatter> sink_formatter) {
  formatter_ = std::move(sink_formatter);
}

void BaseSink::flush() {}

void BaseSink::Reopen(ReopenMode) {}

std::mutex& BaseSink::GetMutex() { return mutex_; }

BaseSink::~BaseSink() = default;

}  // namespace logging::impl

USERVER_NAMESPACE_END
