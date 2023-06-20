#include "base_sink.hpp"

#include <spdlog/pattern_formatter.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
BaseSink::BaseSink()
    : formatter_{std::make_unique<spdlog::pattern_formatter>()} {}

void BaseSink::Log(const spdlog::details::log_msg& msg) {
  std::lock_guard lock{mutex_};

  spdlog::memory_buf_t formatted;
  formatter_->format(msg, formatted);

  Write({formatted.data(), formatted.size()});
}

void BaseSink::SetPattern(const std::string& pattern) {
  SetFormatter(std::make_unique<spdlog::pattern_formatter>(pattern));
}

void BaseSink::SetFormatter(std::unique_ptr<spdlog::formatter> sink_formatter) {
  formatter_ = std::move(sink_formatter);
}

void BaseSink::Flush() {}

void BaseSink::Reopen(ReopenMode) {}

void BaseSink::SetLevel(Level log_level) { level_.store(log_level); }

Level BaseSink::GetLevel() const { return level_.load(); }

bool BaseSink::IsShouldLog(Level msg_level) const {
  return msg_level >= level_.load();
}

std::mutex& BaseSink::GetMutex() { return mutex_; }

BaseSink::~BaseSink() = default;

}  // namespace logging::impl

USERVER_NAMESPACE_END
