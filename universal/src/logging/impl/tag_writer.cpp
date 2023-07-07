#include <userver/logging/impl/tag_writer.hpp>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

struct ExtraValue final {
  const LogExtra::Value& value;
};

LogHelper& operator<<(LogHelper& lh, ExtraValue value) noexcept {
  std::visit([&lh](const auto& contents) { lh << contents; }, value.value);
  return lh;
}

}  // namespace

void ThrowInvalidEscapedTagKey(std::string_view key) {
  UINVARIANT(false, fmt::format("TagKey({}) contains an invalid character. Use "
                                "RuntimeTagKey for such keys",
                                key));
}

std::string_view TagKey::GetEscapedKey() const noexcept { return escaped_key_; }

RuntimeTagKey::RuntimeTagKey(std::string_view unescaped_key)
    : unescaped_key_(unescaped_key) {}

std::string_view RuntimeTagKey::GetUnescapedKey() const noexcept {
  return unescaped_key_;
}

void TagWriter::PutLogExtra(const LogExtra& extra) {
  for (const auto& item : *extra.extra_) {
    PutTag(RuntimeTagKey{item.first}, ExtraValue{item.second.GetValue()});
  }
}

TagWriter::TagWriter(LogHelper& lh) noexcept : lh_(lh) {}

}  // namespace logging::impl

USERVER_NAMESPACE_END
