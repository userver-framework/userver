#include <userver/logging/impl/tag_writer.hpp>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <logging/log_helper_impl.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

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
  for (const auto& [key, value] : *extra.extra_) {
    std::visit([&key, this](auto& value) { PutTag(RuntimeTagKey{key}, value); },
               value.GetValue());
  }
}

void TagWriter::ExtendLogExtra(const LogExtra& extra) {
  lh_.pimpl_->GetLogExtra().Extend(extra);
}

TagWriter::TagWriter(LogHelper& lh) noexcept : lh_(lh) {}

void TagWriter::PutKey(TagKey key) {
  lh_.pimpl_->PutRawKey(key.GetEscapedKey());
}

void TagWriter::PutKey(RuntimeTagKey key) {
  lh_.pimpl_->PutKey(key.GetUnescapedKey());
}

void TagWriter::MarkValueEnd() noexcept { lh_.pimpl_->MarkValueEnd(); }

void TagWriter::PutOptionalOpenCloseSeparator() {
  lh_.pimpl_->PutOptionalOpenCloseSeparator();
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
