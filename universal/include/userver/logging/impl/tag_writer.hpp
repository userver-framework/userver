#pragma once

#include <string_view>
#include <type_traits>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

[[noreturn]] void ThrowInvalidEscapedTagKey(std::string_view key);

class TagKey final {
 public:
  template <typename StringType, typename Enabled = std::enable_if_t<
                                     !std::is_same_v<StringType, TagKey>>>
  USERVER_IMPL_CONSTEVAL /*implicit*/ TagKey(const StringType& escaped_key);

  std::string_view GetEscapedKey() const noexcept;

 private:
  std::string_view escaped_key_;
};

class RuntimeTagKey final {
 public:
  explicit RuntimeTagKey(std::string_view unescaped_key);

  std::string_view GetUnescapedKey() const noexcept;

 private:
  std::string_view unescaped_key_;
};

// Allows to add tags directly to the logged message, bypassing LogExtra.
class TagWriter {
 public:
  template <typename T>
  void PutTag(TagKey key, const T& value);

  template <typename T>
  void PutTag(RuntimeTagKey key, const T& value);

  // The tags must not be duplicated in other Put* calls.
  void PutLogExtra(const LogExtra& extra);

  // Copies the tags to the internal LogExtra. They will be deduplicated
  // automatically.
  void ExtendLogExtra(const LogExtra& extra);

 private:
  friend class logging::LogHelper;

  explicit TagWriter(LogHelper& lh) noexcept;

  void PutKey(TagKey key);
  void PutKey(RuntimeTagKey key);

  void MarkValueEnd() noexcept;

  LogHelper& lh_;
};

constexpr bool DoesTagNeedEscaping(std::string_view key) noexcept {
  for (const char c : key) {
    const bool needs_no_escaping_in_all_formats =
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '/';
    if (!needs_no_escaping_in_all_formats) {
      return true;
    }
  }
  return false;
}

template <typename StringType, typename Enabled>
USERVER_IMPL_CONSTEVAL TagKey::TagKey(const StringType& escaped_key)
    : escaped_key_(escaped_key) {
  if (DoesTagNeedEscaping(escaped_key_)) {
    ThrowInvalidEscapedTagKey(escaped_key_);
  }
}

template <typename T>
void TagWriter::PutTag(TagKey key, const T& value) {
  PutKey(key);
  lh_ << value;
  MarkValueEnd();
}

template <typename T>
void TagWriter::PutTag(RuntimeTagKey key, const T& value) {
  PutKey(key);
  lh_ << value;
  MarkValueEnd();
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
