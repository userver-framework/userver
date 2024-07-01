#pragma once

/// @file userver/utest/log_capture_fixture.hpp
/// @brief @copybrief utest::LogCaptureFixture

#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/default_logger_fixture.hpp>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace impl {
class ToStringLogger;
}  // namespace impl

/// @brief Represents single log record, typically written via `LOG_*` macros.
/// @see @ref utest::LogCaptureLogger
/// @see @ref utest::LogCaptureFixture
class LogRecord final {
 public:
  /// @returns decoded text of the log record
  /// @throws if no 'text' tag in the log
  const std::string& GetText() const;

  /// @returns decoded value of the tag in the log record
  /// @throws if no such tag in the log
  const std::string& GetTag(std::string_view key) const;

  /// @returns decoded value of the tag in the log record, or `std::nullopt`
  const std::string* GetTagOptional(std::string_view key) const;

  /// @returns serialized log record
  const std::string& GetLogRaw() const;

  /// @returns the log level of the record
  logging::Level GetLevel() const;

  /// @cond
  // For internal use only.
  LogRecord(utils::impl::InternalTag, logging::Level level,
            std::string&& log_raw);
  /// @endcond

 private:
  logging::Level level_;
  std::string log_raw_;
  std::vector<std::pair<std::string, std::string>> tags_;
};

std::ostream& operator<<(std::ostream&, const LogRecord& data);

std::ostream& operator<<(std::ostream&, const std::vector<LogRecord>& data);

/// @brief A mocked logger that stores the log records in memory.
/// @see @ref utest::LogCaptureFixture
class LogCaptureLogger final {
 public:
  explicit LogCaptureLogger(logging::Format format = logging::Format::kRaw);

  /// @returns the mocked logger.
  logging::LoggerPtr GetLogger() const;

  /// @returns all collected logs.
  std::vector<LogRecord> GetAll() const;

  /// @returns the single collected log record, then clears logs.
  /// @throws std::runtime_error if there are zero or multiple log records.
  LogRecord ExtractSingle();

  /// @returns logs filtered by (optional) text substring and (optional) tags
  /// substrings.
  std::vector<LogRecord> Filter(
      std::string_view text_substring,
      utils::span<std::pair<std::string_view, std::string_view>>
          tag_substrings = {}) const;

  /// @returns logs filtered by an arbitrary predicate.
  std::vector<LogRecord> Filter(
      utils::function_ref<bool(const LogRecord&)> predicate) const;

  /// @brief Discards the collected logs.
  void Clear() noexcept;

  /// @brief Logs @a value as-if using `LOG_*`, then extracts the log text.
  template <typename T>
  std::string ToStringViaLogging(const T& value) {
    Clear();
    LOG_CRITICAL() << value;
    return ExtractSingle().GetText();
  }

 private:
  utils::SharedRef<impl::ToStringLogger> logger_;
};

/// @brief Fixture that allows to capture and extract log written into the
/// default logger.
/// @see @ref utest::LogCaptureLogger
template <typename Base = ::testing::Test>
class LogCaptureFixture : public DefaultLoggerFixture<Base> {
 protected:
  LogCaptureFixture() {
    DefaultLoggerFixture<Base>::SetDefaultLogger(logger_.GetLogger());
  }

  LogCaptureLogger& GetLogCapture() { return logger_; }

 private:
  LogCaptureLogger logger_;
};

}  // namespace utest

USERVER_NAMESPACE_END
