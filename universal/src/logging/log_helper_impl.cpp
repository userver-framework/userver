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

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

char GetSeparatorFromLogger(LoggerRef logger) {
  switch (logger.GetFormat()) {
    case Format::kTskv:
    case Format::kRaw:
      return '=';
    case Format::kLtsv:
      return ':';
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
      key_value_separator_(GetSeparatorFromLogger(*logger_)) {
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
  }
  UASSERT_MSG(false, "Invalid value of Format enum");
}

void LogHelper::Impl::PutMessageEnd() { msg_.push_back('\n'); }

void LogHelper::Impl::PutKey(std::string_view key) {
  if (!utils::encoding::ShouldKeyBeEscaped(key)) {
    PutRawKey(key);
  } else {
    UASSERT(!std::exchange(is_within_value_, true));
    CheckRepeatedKeys(key);
    msg_.push_back(utils::encoding::kTskvPairsSeparator);
    utils::encoding::EncodeTskv(
        msg_, key, utils::encoding::EncodeTskvMode::kKeyReplacePeriod);
    msg_.push_back(key_value_separator_);
  }
}

void LogHelper::Impl::PutRawKey(std::string_view key) {
  UASSERT(!std::exchange(is_within_value_, true));
  CheckRepeatedKeys(key);
  const auto old_size = msg_.size();
  msg_.resize(old_size + 1 + key.size() + 1);

  auto* position = msg_.data() + old_size;
  *(position++) = utils::encoding::kTskvPairsSeparator;
  key.copy(position, key.size());
  position += key.size();
  *(position++) = key_value_separator_;
}

void LogHelper::Impl::PutValuePart(std::string_view value) {
  UASSERT(is_within_value_);
  utils::encoding::EncodeTskv(msg_, value,
                              utils::encoding::EncodeTskvMode::kValue);
}

void LogHelper::Impl::PutValuePart(char text_part) {
  UASSERT(is_within_value_);
  utils::encoding::EncodeTskv(fmt::appender(msg_), text_part,
                              utils::encoding::EncodeTskvMode::kValue);
}

LogBuffer& LogHelper::Impl::GetBufferForRawValuePart() noexcept {
  UASSERT(is_within_value_);
  return msg_;
}

void LogHelper::Impl::MarkValueEnd() noexcept {
  UASSERT(std::exchange(is_within_value_, false));
}

void LogHelper::Impl::StartText() {
  PutRawKey("text");
  initial_length_ = msg_.size();
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

}  // namespace logging

USERVER_NAMESPACE_END
