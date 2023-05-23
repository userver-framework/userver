#include "log_helper_impl.hpp"

#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

char GetSeparatorFromLogger(LoggerCRef logger) {
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

LogHelper::Impl::int_type LogHelper::Impl::BufferStd::overflow(int_type c) {
  return impl_.overflow(c);
}

std::streamsize LogHelper::Impl::BufferStd::xsputn(const char_type* s,
                                                   std::streamsize n) {
  return impl_.xsputn(s, n);
}

// Logging is setuped in components::Run and no logging happens before
// that. So at this point the logging is initialized.
LogHelper::Impl::Impl(LoggerCRef logger, Level level) noexcept
    : logger_(&logger),
      level_(level),
      key_value_separator_(GetSeparatorFromLogger(*logger_)) {
  static_assert(sizeof(LogHelper::Impl) < 4096,
                "Structures with size more than 4096 would consume at least "
                "8KB memory in allocator.");
}

std::streamsize LogHelper::Impl::xsputn(const char_type* s, std::streamsize n) {
  switch (encode_mode_) {
    case Encode::kNone:
      msg_.append(s, s + n);
      break;
    case Encode::kValue: {
      utils::encoding::EncodeTskv(msg_, std::string_view(s, n),
                                  utils::encoding::EncodeTskvMode::kValue);
      break;
    }
    case Encode::kKeyReplacePeriod:
      if (!utils::encoding::ShouldKeyBeEscaped({s, static_cast<size_t>(n)})) {
        msg_.append(s, s + n);
      } else {
        utils::encoding::EncodeTskv(
            msg_, std::string_view(s, n),
            utils::encoding::EncodeTskvMode::kKeyReplacePeriod);
      }
      break;
  }

  return n;
}

LogHelper::Impl::int_type LogHelper::Impl::overflow(int_type c) {
  if (c == std::streambuf::traits_type::eof()) return c;
  Put(c);
  return c;
}

void LogHelper::Impl::Put(char_type c) {
  switch (encode_mode_) {
    case Encode::kNone:
      msg_.push_back(c);
      break;
    case Encode::kValue:
      utils::encoding::EncodeTskv(std::back_inserter(msg_),
                                  static_cast<char>(c),
                                  utils::encoding::EncodeTskvMode::kValue);
      break;
    case Encode::kKeyReplacePeriod:
      utils::encoding::EncodeTskv(
          std::back_inserter(msg_), static_cast<char>(c),
          utils::encoding::EncodeTskvMode::kKeyReplacePeriod);
      break;
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

void LogHelper::Impl::MarkTextBegin() {
  UASSERT_MSG(initial_length_ == 0, "MarkTextBegin must only be called once");
  initial_length_ = msg_.size();
}

void LogHelper::Impl::MarkAsBroken() noexcept { logger_ = nullptr; }

bool LogHelper::Impl::IsBroken() const noexcept { return !logger_; }

}  // namespace logging

USERVER_NAMESPACE_END
