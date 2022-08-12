#include "log_helper_impl.hpp"

#include <logging/spdlog.hpp>

#include <logging/logger_with_info.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

class PutCharFmtBuffer final {
 public:
  template <size_t Size>
  void operator()(fmt::basic_memory_buffer<char, Size>& to, char ch) const {
    to.push_back(ch);
  }
};

char GetSeparatorFromLogger(const LoggerPtr& logger_ptr) {
  if (!logger_ptr) {
    return '?';  // Won't be logged
  }

  const auto& logger = *logger_ptr;

  switch (logger.format) {
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

// SetDefaultLogger is called in components::Run and no logging happens before
// that. So at this point the logging is initialized and
// DefaultLogger()->name() shuld not throw.
LogHelper::Impl::Impl(LoggerPtr logger, Level level) noexcept
    : logger_(std::move(logger)),
      level_(level),
      key_value_separator_(GetSeparatorFromLogger(logger_)) {
  static_assert(sizeof(LogHelper::Impl) < 4096,
                "Structures with size more than 4096 would consume at least "
                "8KB memory in allocator.");
}

std::streamsize LogHelper::Impl::xsputn(const char_type* s, std::streamsize n) {
  switch (encode_mode_) {
    case Encode::kNone:
      msg_.append(s, s + n);
      break;
    case Encode::kValue:
      msg_.reserve(msg_.size() + n);
      utils::encoding::EncodeTskv(msg_, s, s + n,
                                  utils::encoding::EncodeTskvMode::kValue,
                                  PutCharFmtBuffer{});
      break;
    case Encode::kKeyReplacePeriod:
      msg_.reserve(msg_.size() + n);
      utils::encoding::EncodeTskv(
          msg_, s, s + n, utils::encoding::EncodeTskvMode::kKeyReplacePeriod,
          PutCharFmtBuffer{});
      break;
  }

  return n;
}

LogHelper::Impl::int_type LogHelper::Impl::overflow(int_type c) {
  if (c == std::streambuf::traits_type::eof()) return c;

  switch (encode_mode_) {
    case Encode::kNone:
      msg_.push_back(c);
      break;
    case Encode::kValue:
      utils::encoding::EncodeTskv(msg_, static_cast<char>(c),
                                  utils::encoding::EncodeTskvMode::kValue,
                                  PutCharFmtBuffer{});
      break;
    case Encode::kKeyReplacePeriod:
      utils::encoding::EncodeTskv(
          msg_, static_cast<char>(c),
          utils::encoding::EncodeTskvMode::kKeyReplacePeriod,
          PutCharFmtBuffer{});
      break;
  }

  return c;
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
  std::string_view message(msg_.data(), msg_.size());
  logger_->ptr->log(static_cast<spdlog::level::level_enum>(level_), message);
}

void LogHelper::Impl::MarkTextBegin() {
  UASSERT_MSG(initial_length_ == 0, "MarkTextBegin must only be called once");
  initial_length_ = msg_.size();
}

void LogHelper::Impl::MarkAsBroken() noexcept { logger_.reset(); }

bool LogHelper::Impl::IsBroken() const noexcept { return !logger_; }

}  // namespace logging

USERVER_NAMESPACE_END
