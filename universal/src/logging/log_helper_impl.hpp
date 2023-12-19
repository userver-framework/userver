#pragma once

#include <optional>
#include <ostream>
#include <unordered_set>

#include <fmt/format.h>

#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

inline constexpr std::size_t kInitialLogBufferSize = 1500;
using LogBuffer = fmt::basic_memory_buffer<char, kInitialLogBufferSize>;

struct LogHelper::InternalTag final {};

class LogHelper::Impl final {
 public:
  explicit Impl(LoggerRef logger, Level level) noexcept;

  bool IsStreamInitialized() const noexcept { return !!lazy_stream_; }
  std::ostream& Stream() { return GetLazyInitedStream().ostr; }

  void PutMessageBegin();
  void PutMessageEnd();

  // Closes the previous tag value, puts the new tag key, opens the new value.
  void PutKey(std::string_view key);
  void PutRawKey(std::string_view key);

  void PutValuePart(std::string_view value);
  void PutValuePart(char text_part);
  LogBuffer& GetBufferForRawValuePart() noexcept;

  bool IsWithinValue() const noexcept { return is_within_value_; }
  void MarkValueEnd() noexcept;

  LogExtra& GetLogExtra() { return extra_; }

  void StartText();

  std::size_t GetTextSize() const { return msg_.size() - initial_length_; }

  void LogTheMessage() const;

  void MarkAsBroken() noexcept;

  bool IsBroken() const noexcept;

 private:
  class BufferStd final : public std::streambuf {
   public:
    using char_type = std::streambuf::char_type;
    using int_type = std::streambuf::int_type;

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

  void CheckRepeatedKeys(std::string_view raw_key);

  impl::LoggerBase* logger_;
  const Level level_;
  const char key_value_separator_;
  LogBuffer msg_;
  std::optional<LazyInitedStream> lazy_stream_;
  LogExtra extra_;
  std::size_t initial_length_{0};
  bool is_within_value_{false};
  std::optional<std::unordered_set<std::string>> debug_tag_keys_;
};

}  // namespace logging

USERVER_NAMESPACE_END
