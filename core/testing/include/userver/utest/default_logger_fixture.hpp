#pragma once

/// @file userver/utest/default_logger_fixture.hpp
/// @brief @copybrief utest::DefaultLoggerFixture

#include <vector>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @brief Fixture that allows to set the default logger and manages its
/// lifetime.
template <class Base>
class DefaultLoggerFixture : public Base {
 public:
  static void TearDownTestSuite() {
    Base::TearDownTestSuite();
    once_used_loggers_.clear();
  }

 protected:
  /// Set the default logger and postpone its destruction till the coroutine
  /// engine stops
  void SetDefaultLogger(logging::LoggerPtr new_logger) {
    UASSERT(new_logger);

    if (!logger_initial_) {
      logger_initial_ = &logging::impl::DefaultLoggerRef();
      level_initial_ = logging::GetDefaultLoggerLevel();
    }

    logging::impl::SetDefaultLoggerRef(*new_logger);

    // Logger could be used by the ev-thread, so we postpone the
    // destruction of logger for the lifetime of coroutine engine.
    once_used_loggers_.emplace_back(std::move(new_logger));
  }

  ~DefaultLoggerFixture() override {
    if (logger_initial_) {
      logging::impl::SetDefaultLoggerRef(*logger_initial_);
      logging::SetDefaultLoggerLevel(level_initial_);
    }
  }

 private:
  logging::impl::LoggerBase* logger_initial_{nullptr};
  logging::Level level_initial_{};

  static inline std::vector<logging::LoggerPtr> once_used_loggers_;
};

}  // namespace utest

USERVER_NAMESPACE_END
