#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <protobuf/json/impl/convert_utils.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/formats/json/string_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

/// @brief Handler for @ref ProtoMessageVisitor that writes a JSON string directly into a `formats::json::StringBuilder`
/// (SAX-style, no intermediate DOM). Honors an optional output size limit: once the produced JSON reaches `limit`
/// bytes the visitor stops (see @ref LimitReached), and oversized string/bytes values are capped to the remaining
/// budget so a huge field is not escaped/encoded in full.
class StringWriter final {
public:
    class ObjectGuard final {
    public:
        explicit ObjectGuard(StringWriter& string_writer)
            : guard_(string_writer.sb_)
        {}

    private:
        formats::json::StringBuilder::ObjectGuard guard_;
    };

    class ArrayGuard final {
    public:
        explicit ArrayGuard(StringWriter& string_writer)
            : guard_(string_writer.sb_)
        {}

    private:
        formats::json::StringBuilder::ArrayGuard guard_;
    };

    explicit StringWriter(std::size_t limit)
        : limit_(limit)
    {}

    void Key(std::string_view key) { sb_.Key(key); }

    void Null() { sb_.WriteNull(); }
    void Bool(bool value) { sb_.WriteBool(value); }
    void Int32(std::int32_t value) { sb_.WriteInt64(value); }
    void UInt32(std::uint32_t value) { sb_.WriteUInt64(value); }
    // ProtoJSON represents 64-bit integers as strings.
    void Int64(std::int64_t value) { sb_.WriteString(std::to_string(value)); }
    void UInt64(std::uint64_t value) { sb_.WriteString(std::to_string(value)); }
    void Float(float value) { WriteDouble(static_cast<double>(value)); }
    void Double(double value) { WriteDouble(value); }
    void String(std::string_view value) { sb_.WriteString(Cap(value)); }
    void Bytes(std::string_view bytes) { sb_.WriteString(crypto::base64::Base64Encode(CapBytes(bytes))); }

    [[nodiscard]] bool LimitReached() const { return limit_ <= sb_.GetStringView().size(); }

    [[nodiscard]] std::string GetString() const { return sb_.GetString(); }

private:
    void WriteDouble(double value) {
        if (std::isnan(value)) {
            sb_.WriteString(kNan);
        } else if (std::isinf(value)) {
            sb_.WriteString(value < 0 ? kNegativeInf : kPositiveInf);
        } else {
            sb_.WriteDouble(value);
        }
    }

    // Remaining output budget in bytes
    [[nodiscard]] std::size_t RemainingBudget() const {
        const std::size_t produced = sb_.GetStringView().size();
        return limit_ <= produced ? 0 : limit_ - produced;
    }

    // Cap a string value to the remaining budget: every source character yields at least one output byte, so once the
    // limit is (nearly) reached only a prefix could fit anyway. Keeps serialization O(limit) for huge string fields.
    [[nodiscard]] std::string_view Cap(std::string_view value) const {
        const std::size_t remaining = RemainingBudget();
        if (remaining < value.size()) {
            return value.substr(0, remaining);
        }
        return value;
    }

    // Same idea for bytes: cap the raw input before the (expanding) base64 encoding.
    [[nodiscard]] std::string_view CapBytes(std::string_view bytes) const {
        // base64 emits 4 output bytes per 3 input bytes
        const std::size_t remaining = RemainingBudget() / 4 * 3 + 3;
        if (remaining < bytes.size()) {
            return bytes.substr(0, remaining);
        }
        return bytes;
    }

    formats::json::StringBuilder sb_;
    std::size_t limit_;
};

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
