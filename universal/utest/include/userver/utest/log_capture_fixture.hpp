#pragma once

/// @file userver/utest/log_capture_fixture.hpp
/// @brief @copybrief utest::LogCaptureFixture

#include <mutex>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/utest/default_logger_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace impl {

/// @brief Thread-safe logger that allows to capture and extract log.
class ToStringLogger : public logging::impl::LoggerBase {
 public:
  ToStringLogger() noexcept : logging::impl::LoggerBase(logging::Format::kRaw) {
    logging::impl::LoggerBase::SetLevel(logging::Level::kInfo);
  }

  void Log(logging::Level level, std::string_view str) override {
    const std::lock_guard lock{log_mutex_};
    log_ += fmt::format("level={}\t{}", logging::ToString(level), str);
  }

  std::string ExtractLog() {
    const std::lock_guard lock{log_mutex_};
    return std::exchange(log_, std::string{});
  }

 private:
  std::string log_;
  std::mutex log_mutex_;
};

}  // namespace impl

/// @brief Fixture that allows to capture and extract log.
template <typename Base = ::testing::Test>
class LogCaptureFixture : public utest::DefaultLoggerFixture<Base> {
 protected:
  LogCaptureFixture() : logger_(std::make_shared<impl::ToStringLogger>()) {
    utest::DefaultLoggerFixture<Base>::SetDefaultLogger(logger_);
  }

  /// Extract and clear current log
  std::string ExtractRawLog() { return logger_->ExtractLog(); }

 private:
  std::shared_ptr<impl::ToStringLogger> logger_;
};

}  // namespace utest

USERVER_NAMESPACE_END
