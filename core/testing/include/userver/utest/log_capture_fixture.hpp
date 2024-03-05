#pragma once

/// @file userver/utest/log_capture_fixture.hpp
/// @brief @copybrief utest::LogCaptureFixture

#include <userver/concurrent/variable.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/utest/default_logger_fixture.hpp>
#include <userver/utest/utest.hpp>

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
    auto locked_ptr = log_.Lock();
    *locked_ptr += fmt::format("level={}\t{}", logging::ToString(level), str);
  }

  std::string ExtractLog() {
    auto locked_ptr = log_.Lock();
    return std::exchange(*locked_ptr, std::string{});
  }

 private:
  concurrent::Variable<std::string, std::mutex> log_;
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
