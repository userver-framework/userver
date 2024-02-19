#include "log_helper_impl.hpp"

#include <array>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/tskv.hpp>
#include "encoding_json.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

char GetKeyValueSeparatorFromLogger(LoggerRef logger) {
  switch (logger.GetFormat()) {
    case Format::kTskv:
    case Format::kRaw:
      return '=';
    case Format::kLtsv:
    case Format::kJson:
      return ':';
  }

  UINVARIANT(false, "Invalid logging::Format enum value");
}

char GetItemSeparatorFromLogger(LoggerRef logger) {
  switch (logger.GetFormat()) {
    case Format::kTskv:
    case Format::kRaw:
    case Format::kLtsv:
      return '\t';
    case Format::kJson:
      return ',';
  }

  UINVARIANT(false, "Invalid logging::Format enum value");
}

char GetOpenCloseSeparatorFromLogger(LoggerRef logger) {
  switch (logger.GetFormat()) {
    case Format::kTskv:
    case Format::kRaw:
    case Format::kLtsv:
      return '\0';
    case Format::kJson:
      return '"';
  }

  UINVARIANT(false, "Invalid logging::Format enum value");
}

using TimePoint = std::chrono::system_clock::time_point;

auto FractionalMicroseconds(TimePoint time) noexcept {
  return std::chrono::time_point_cast<std::chrono::microseconds>(time)
             .time_since_epoch()
             .count() %
         1'000'000;
}

using SecondsTimePoint =
    std::chrono::time_point<TimePoint::clock, std::chrono::seconds>;
constexpr std::string_view kTimeTemplate = "0000-00-00T00:00:00";

struct TimeString final {
  char data[kTimeTemplate.size()]{};

  std::string_view ToStringView() const noexcept {
    return {data, std::size(data)};
  }
};

struct CachedTime final {
  SecondsTimePoint time{};
  TimeString string{};
};

compiler::ThreadLocal local_cached_time = [] { return CachedTime{}; };

TimeString GetCurrentTimeString(TimePoint now) noexcept {
  auto cached_time = local_cached_time.Use();

  const auto rounded_now =
      std::chrono::time_point_cast<std::chrono::seconds>(now);
  if (rounded_now != cached_time->time) {
    fmt::format_to(cached_time->string.data, FMT_COMPILE("{:%FT%T}"),
                   fmt::localtime(std::chrono::system_clock::to_time_t(now)));
    cached_time->time = rounded_now;
  }
  return cached_time->string;
}

}  // namespace

auto LogHelper::Impl::BufferStd::overflow(int_type c) -> int_type {
  if (c == std::streambuf::traits_type::eof()) return c;
  impl_.PutValuePart(static_cast<char>(c));
  return c;
}

std::streamsize LogHelper::Impl::BufferStd::xsputn(const char_type* s,
                                                   std::streamsize n) {
  impl_.PutValuePart(std::string_view(s, n));
  return n;
}

LogHelper::Impl::Impl(LoggerRef logger, Level level) noexcept
    : logger_(&logger),
      level_(std::max(level, logger_->GetLevel())),
      key_value_separator_(GetKeyValueSeparatorFromLogger(*logger_)),
      item_separator_(GetItemSeparatorFromLogger(*logger_)),
      open_close_separator_optional_(
          GetOpenCloseSeparatorFromLogger(*logger_)) {
  static_assert(sizeof(LogHelper::Impl) < 4096,
                "Structures with size more than 4096 would consume at least "
                "8KB memory in allocator.");
  if constexpr (utils::impl::kEnableAssert) {
    debug_tag_keys_.emplace();
  }
}

void LogHelper::Impl::PutMessageBegin() {
  UASSERT(msg_.size() == 0);

  switch (logger_->GetFormat()) {
    case Format::kTskv: {
      constexpr std::string_view kTemplate =
          "tskv\ttimestamp=0000-00-00T00:00:00.000000\tlevel=";
      const auto now = TimePoint::clock::now();
      const auto level_string = logging::ToUpperCaseString(level_);
      msg_.resize(kTemplate.size() + level_string.size());
      fmt::format_to(msg_.data(),
                     FMT_COMPILE("tskv\ttimestamp={}.{:06}\tlevel={}"),
                     GetCurrentTimeString(now).ToStringView(),
                     FractionalMicroseconds(now), level_string);
      return;
    }
    case Format::kLtsv: {
      constexpr std::string_view kTemplate =
          "timestamp:0000-00-00T00:00:00.000000\tlevel:";
      const auto now = TimePoint::clock::now();
      const auto level_string = logging::ToUpperCaseString(level_);
      msg_.resize(kTemplate.size() + level_string.size());
      fmt::format_to(msg_.data(), FMT_COMPILE("timestamp:{}.{:06}\tlevel:{}"),
                     GetCurrentTimeString(now).ToStringView(),
                     FractionalMicroseconds(now), level_string);
      return;
    }
    case Format::kRaw: {
      msg_.append(std::string_view{"tskv"});
      return;
    }
    case Format::kJson: {
      constexpr std::string_view kTemplate =
          R"({"timestamp":"0000-00-00T00:00:00.000000","level":"")";
      const auto now = TimePoint::clock::now();
      const auto level_string = logging::ToUpperCaseString(level_);
      msg_.resize(kTemplate.size() + level_string.size());
      fmt::format_to(msg_.data(),
                     // double `{` at the beginning for escaping:
                     // https://stackoverflow.com/a/68207254
                     FMT_COMPILE(R"({{"timestamp":"{}.{:06}","level":"{}")"),
                     GetCurrentTimeString(now).ToStringView(),
                     FractionalMicroseconds(now), level_string);
      return;
    }
  }
  UASSERT_MSG(false, "Invalid value of Format enum");
}

void LogHelper::Impl::PutMessageEnd() {
  if (logger_->GetFormat() == Format::kJson) {
    msg_.push_back('}');
  }
  msg_.push_back('\n');
}

void LogHelper::Impl::PutKey(std::string_view key) {
  // make sure that we have ended writing the "text" value
  StopText();

  // `utils::encoding::ShouldKeyBeEscaped` works only with non json format
  if (!utils::encoding::ShouldKeyBeEscaped(key) &&
      logger_->GetFormat() != Format::kJson) {
    PutRawKey(key);
  } else {
    UASSERT(!std::exchange(is_within_value_, true));
    CheckRepeatedKeys(key);

    PutItemSeparator();
    PutOptionalOpenCloseSeparator();
    if (logger_->GetFormat() == Format::kJson) {
      EncodeJson(msg_, key);
    } else {
      utils::encoding::EncodeTskv(
          msg_, key, utils::encoding::EncodeTskvMode::kKeyReplacePeriod);
    }
    PutOptionalOpenCloseSeparator();
    PutKeyValueSeparator();
  }
}

void LogHelper::Impl::PutRawKey(std::string_view key) {
  // make sure that we have ended writing the "text" value
  StopText();

  UASSERT(!std::exchange(is_within_value_, true));
  CheckRepeatedKeys(key);
  msg_.reserve(msg_.size() + key.size() + 4);

  PutItemSeparator();
  PutOptionalOpenCloseSeparator();
  msg_.append(key);
  PutOptionalOpenCloseSeparator();
  PutKeyValueSeparator();
}

void LogHelper::Impl::PutValuePart(std::string_view value) {
  UASSERT(is_within_value_);
  if (logger_->GetFormat() == Format::kJson) {
    EncodeJson(msg_, value);
  } else {
    utils::encoding::EncodeTskv(msg_, value,
                                utils::encoding::EncodeTskvMode::kValue);
  }
}

void LogHelper::Impl::PutValuePart(char text_part) {
  UASSERT(is_within_value_);
  if (logger_->GetFormat() == Format::kJson) {
    EncodeJson(msg_, std::string_view{&text_part, 1});
  } else {
    utils::encoding::EncodeTskv(fmt::appender(msg_), text_part,
                                utils::encoding::EncodeTskvMode::kValue);
  }
}

void LogHelper::Impl::PutKeyValueSeparator() {
  msg_.push_back(key_value_separator_);
}

void LogHelper::Impl::PutItemSeparator() { msg_.push_back(item_separator_); }

void LogHelper::Impl::PutOptionalOpenCloseSeparator() {
  if (open_close_separator_optional_) {
    msg_.push_back(open_close_separator_optional_);
  }
}

LogBuffer& LogHelper::Impl::GetBufferForRawValuePart() noexcept {
  UASSERT(is_within_value_);
  return msg_;
}

void LogHelper::Impl::MarkValueEnd() noexcept {
  UASSERT(std::exchange(is_within_value_, false));
}

void LogHelper::Impl::StartText() {
  UASSERT(is_text_finished_ == false);
  PutRawKey("text");
  PutOptionalOpenCloseSeparator();
  initial_length_ = msg_.size();
  UASSERT(is_text_finished_ == false);
}

void LogHelper::Impl::StopText() {
  if (initial_length_ == 0) {
    // text hasn't started yet
    return;
  }
  if (!std::exchange(is_text_finished_, true)) {
    PutOptionalOpenCloseSeparator();
  }
}

LogHelper::Impl::LazyInitedStream& LogHelper::Impl::GetLazyInitedStream() {
  if (!IsStreamInitialized()) {
    lazy_stream_.emplace(*this);
  }

  return *lazy_stream_;
}

void LogHelper::Impl::LogTheMessage() const {
  if (IsBroken()) {
    return;
  }

  UASSERT(logger_);
  const std::string_view message(msg_.data(), msg_.size());
  logger_->Log(level_, message);
}

void LogHelper::Impl::MarkAsBroken() noexcept { logger_ = nullptr; }

bool LogHelper::Impl::IsBroken() const noexcept { return !logger_; }

void LogHelper::Impl::CheckRepeatedKeys(
    [[maybe_unused]] std::string_view raw_key) {
  UASSERT_MSG(debug_tag_keys_->insert(std::string{raw_key}).second,
              fmt::format("Repeated tag in logs: '{}'", raw_key));
}

void LogHelper::Impl::WriteRawJsonValue(std::string_view json) {
  UASSERT(is_within_value_);

  if (logger_->GetFormat() == Format::kJson) {
    msg_.append(json);
  } else {
    utils::encoding::EncodeTskv(msg_, json,
                                utils::encoding::EncodeTskvMode::kValue);
  }
}

}  // namespace logging

USERVER_NAMESPACE_END
