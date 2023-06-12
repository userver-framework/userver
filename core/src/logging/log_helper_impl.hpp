#pragma once

#include <optional>
#include <ostream>

#include <fmt/format.h>

#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

class LogHelper::Impl final {
 public:
  using char_type = std::streambuf::char_type;
  using int_type = std::streambuf::int_type;

  static std::unique_ptr<LogHelper::Impl> Make(LoggerCRef logger, Level level);

  explicit Impl(LoggerCRef logger, Level level) noexcept;

  void SetEncoding(Encode encode_mode) noexcept { encode_mode_ = encode_mode; }
  Encode GetEncoding() const noexcept { return encode_mode_; }

  auto& Message() noexcept { return msg_; }
  std::size_t Capacity() const noexcept { return msg_.capacity(); }

  bool IsStreamInitialized() const noexcept { return !!lazy_stream_; }
  std::ostream& Stream() { return GetLazyInitedStream().ostr; }
  std::streambuf* StreamBuf() { return &GetLazyInitedStream().sbuf; }

  LogExtra& GetLogExtra() { return extra_; }

  std::streamsize xsputn(const char_type* s, std::streamsize n);
  int_type overflow(int_type c);

  void Put(char_type c);
  void PutKeyValueSeparator() { Put(key_value_separator_); }

  void LogTheMessage() const;

  void MarkTextBegin();
  size_t TextSize() const { return msg_.size() - initial_length_; }

  void MarkAsBroken() noexcept;

  bool IsBroken() const noexcept;

 private:
  class BufferStd final : public std::streambuf {
   public:
    explicit BufferStd(Impl& impl) : impl_{impl} {}

   private:
    int_type overflow(int_type c) override;
    std::streamsize xsputn(const char_type* s, std::streamsize n) override;

    Impl& impl_;
  };

  struct LazyInitedStream {
    BufferStd sbuf;
    std::ostream ostr;

    explicit LazyInitedStream(Impl& impl) : sbuf{impl}, ostr(&sbuf) {}
  };

  LazyInitedStream& GetLazyInitedStream();

  static constexpr size_t kOptimalBufferSize = 1500;

  const impl::LoggerBase* logger_;
  const Level level_;
  const char key_value_separator_;
  Encode encode_mode_{Encode::kNone};
  fmt::basic_memory_buffer<char, kOptimalBufferSize> msg_;
  std::optional<LazyInitedStream> lazy_stream_;
  LogExtra extra_;
  size_t initial_length_{0};
};

}  // namespace logging

USERVER_NAMESPACE_END
