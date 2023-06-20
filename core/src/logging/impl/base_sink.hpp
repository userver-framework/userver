#pragma once

#include <mutex>

#include <spdlog/formatter.h>

#include <logging/impl/open_file_helper.hpp>
#include "userver/logging/level.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class BaseSink {
 public:
  virtual ~BaseSink();
  BaseSink();

  void Log(const spdlog::details::log_msg& msg);

  virtual void Flush();

  void SetPattern(const std::string& pattern);

  void SetFormatter(std::unique_ptr<spdlog::formatter> sink_formatter);

  virtual void Reopen(ReopenMode);

  void SetLevel(Level log_level);
  Level GetLevel() const;
  bool IsShouldLog(Level msg_level) const;

 protected:
  virtual void Write(std::string_view log) = 0;
  std::mutex& GetMutex();

 private:
  std::mutex mutex_;
  std::unique_ptr<spdlog::formatter> formatter_;
  std::atomic<Level> level_{Level::kTrace};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
