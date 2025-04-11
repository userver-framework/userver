#include <userver/logging/impl/tag_writer.hpp>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <logging/log_helper_impl.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

void ThrowInvalidEscapedTagKey(std::string_view key) {
    UINVARIANT(
        false,
        fmt::format(
            "TagKey({}) contains an invalid character. Use "
            "RuntimeTagKey for such keys",
            key
        )
    );
}

std::string_view TagKey::GetEscapedKey() const noexcept { return escaped_key_; }

RuntimeTagKey::RuntimeTagKey(std::string_view unescaped_key) : unescaped_key_(unescaped_key) {}

std::string_view RuntimeTagKey::GetUnescapedKey() const noexcept { return unescaped_key_; }

void TagWriter::PutTag(TagKey key, std::string_view value) { lh_.PutSwTag(key.GetEscapedKey(), value); }

void TagWriter::PutLogExtra(const LogExtra& extra) {
    for (const auto& item : *extra.extra_) {
        PutTag(RuntimeTagKey{item.first}, item.second.GetValue());
    }
}

void TagWriter::ExtendLogExtra(const LogExtra& extra) {
    for (const auto& item : *extra.extra_) {
        PutTag(RuntimeTagKey{item.first}, item.second.GetValue());
    }
}

void TagWriter::PutTag(RuntimeTagKey key, std::string_view value) { lh_.PutSwTag(key.GetUnescapedKey(), value); }

TagWriter::TagWriter(LogHelper& lh) noexcept : lh_(lh) {}

}  // namespace logging::impl

USERVER_NAMESPACE_END
