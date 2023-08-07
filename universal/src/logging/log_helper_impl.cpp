#include "log_helper_impl.hpp"

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
      level_(level),
      key_value_separator_(GetSeparatorFromLogger(*logger_)) {
  static_assert(sizeof(LogHelper::Impl) < 4096,
                "Structures with size more than 4096 would consume at least "
                "8KB memory in allocator.");
  if constexpr (utils::impl::kEnableAssert) {
    debug_tag_keys_.emplace();
  }
}

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
