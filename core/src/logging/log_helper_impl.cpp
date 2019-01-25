#include "log_helper_impl.hpp"

#include <utils/encoding/tskv.hpp>

namespace logging {

namespace {

class PutCharFmtBuffer {
 public:
  void operator()(fmt::memory_buffer& to, char ch) const { to.push_back(ch); }
};

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
LogHelper::Impl::Impl(Level level) noexcept
    : msg_(&DefaultLogger()->name(),
           static_cast<spdlog::level::level_enum>(level)),
      encode_mode_{Encode::kNone} {}

std::streamsize LogHelper::Impl::xsputn(const char_type* s, std::streamsize n) {
  switch (encode_mode_) {
    case Encode::kNone:
      msg_.raw.append(s, s + n);
      break;
    case Encode::kValue:
      msg_.raw.reserve(msg_.raw.size() + n);
      utils::encoding::EncodeTskv(msg_.raw, s, s + n,
                                  utils::encoding::EncodeTskvMode::kValue,
                                  PutCharFmtBuffer{});
      break;
    case Encode::kKeyReplacePeriod:
      msg_.raw.reserve(msg_.raw.size() + n);
      utils::encoding::EncodeTskv(
          msg_.raw, s, s + n,
          utils::encoding::EncodeTskvMode::kKeyReplacePeriod,
          PutCharFmtBuffer{});
      break;
  }

  return n;
}

LogHelper::Impl::int_type LogHelper::Impl::overflow(int_type c) {
  if (c == std::streambuf::traits_type::eof()) return c;

  switch (encode_mode_) {
    case Encode::kNone:
      msg_.raw.push_back(c);
      break;
    case Encode::kValue:
      utils::encoding::EncodeTskv(msg_.raw, static_cast<char>(c),
                                  utils::encoding::EncodeTskvMode::kValue,
                                  PutCharFmtBuffer{});
      break;
    case Encode::kKeyReplacePeriod:
      utils::encoding::EncodeTskv(
          msg_.raw, static_cast<char>(c),
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

}  // namespace logging
