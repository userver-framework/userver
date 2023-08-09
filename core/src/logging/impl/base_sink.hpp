#pragma once

#include <atomic>

#include <logging/impl/reopen_mode.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

struct LogMessage final {
  std::string_view payload;
  Level level{logging::Level::kError};
};

class BaseSink {
 public:
  BaseSink(BaseSink&&) = delete;
  BaseSink& operator=(BaseSink&&) = delete;
  virtual ~BaseSink();

  void Log(const LogMessage& message);

  virtual void Flush();

  virtual void Reopen(ReopenMode);

  void SetLevel(Level log_level);
  Level GetLevel() const;
  bool ShouldLog(Level msg_level) const;

 protected:
  BaseSink();

  virtual void Write(std::string_view log) = 0;

 private:
  std::atomic<Level> level_{Level::kTrace};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
