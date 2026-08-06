#include <protobuf/json/impl/write.hpp>

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <google/protobuf/message.h>

#include <protobuf/json/impl/convert_utils.hpp>
#include <protobuf/json/impl/field_error.hpp>
#include <protobuf/json/impl/proto_message_visitor.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/protobuf/json/exceptions.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

namespace {

template <typename T>
[[nodiscard]] formats::json::ValueBuilder GetFloatJsonValue(const T value) {
    if (std::isnan(value)) {
        return formats::json::ValueBuilder{kNan};
    } else if (std::isinf(value)) {
        return formats::json::ValueBuilder{value < 0 ? kNegativeInf : kPositiveInf};
    } else {
        return formats::json::ValueBuilder{value};
    }
}

// Handler for @ref ProtoMessageVisitor that builds a 'formats::json::ValueBuilder' DOM from the SAX-style callbacks.
// Scalars are attached to the current container (object member via 'pending_key_', or array element); nested
// containers are pushed onto a stack and attached to their parent when closed. A value produced while the stack is
// empty becomes the root (well-known types may serialize to a top-level scalar).
class ValueBuilderWriter final {
public:
    class ObjectGuard final {
    public:
        explicit ObjectGuard(ValueBuilderWriter& writer)
            : writer_{writer}
        {
            writer_.OpenContainer(formats::common::Type::kObject);
        }

        ~ObjectGuard() { writer_.CloseContainer(); }

    private:
        ValueBuilderWriter& writer_;
    };

    class ArrayGuard final {
    public:
        explicit ArrayGuard(ValueBuilderWriter& writer)
            : writer_{writer}
        {
            writer_.OpenContainer(formats::common::Type::kArray);
        }

        ~ArrayGuard() { writer_.CloseContainer(); }

    private:
        ValueBuilderWriter& writer_;
    };

    void Key(std::string_view key) { pending_key_ = std::string{key}; }

    void Null() { WriteScalar(formats::json::ValueBuilder{}); }
    void Bool(bool value) { WriteScalar(formats::json::ValueBuilder{value}); }
    void Int32(std::int32_t value) { WriteScalar(formats::json::ValueBuilder{value}); }
    void UInt32(std::uint32_t value) { WriteScalar(formats::json::ValueBuilder{value}); }
    // ProtoJSON represents 64-bit integers as strings.
    void Int64(std::int64_t value) { WriteScalar(formats::json::ValueBuilder{std::to_string(value)}); }
    void UInt64(std::uint64_t value) { WriteScalar(formats::json::ValueBuilder{std::to_string(value)}); }
    void Float(float value) { WriteScalar(GetFloatJsonValue(value)); }
    void Double(double value) { WriteScalar(GetFloatJsonValue(value)); }
    void String(std::string_view value) { WriteScalar(formats::json::ValueBuilder{value}); }
    void Bytes(std::string_view bytes) {
        WriteScalar(formats::json::ValueBuilder{crypto::base64::Base64Encode(std::string{bytes})});
    }

    [[nodiscard]] bool LimitReached() const { return false; }

    [[nodiscard]] formats::json::ValueBuilder ExtractResult() { return std::move(root_); }

private:
    struct Frame {
        formats::json::ValueBuilder builder;
        std::optional<std::string> key;  // key under which this container is attached to its parent (unset in arrays)
        bool is_object;
    };

    void OpenContainer(formats::common::Type type) {
        stack_.push_back(Frame{
            formats::json::ValueBuilder{type},
            std::exchange(pending_key_, std::nullopt),
            type == formats::common::Type::kObject,
        });
    }

    void CloseContainer() {
        Frame frame = std::move(stack_.back());
        stack_.pop_back();
        Place(std::move(frame.key), std::move(frame.builder));
    }

    void WriteScalar(formats::json::ValueBuilder value) {
        Place(std::exchange(pending_key_, std::nullopt), std::move(value));
    }

    void Place(std::optional<std::string> key, formats::json::ValueBuilder value) {
        if (stack_.empty()) {
            root_ = std::move(value);
            return;
        }

        Frame& top = stack_.back();
        if (top.is_object) {
            // An object member is always preceded by a Key() call, so 'pending_key_' must be set here.
            UASSERT_MSG(key.has_value(), "object member is attached without a preceding Key() call");
            top.builder.EmplaceNocheck(std::move(*key), std::move(value));
        } else {
            top.builder.PushBack(std::move(value));
        }
    }

    formats::json::ValueBuilder root_{};
    std::optional<std::string> pending_key_{};
    std::vector<Frame> stack_{};
};

}  // namespace

formats::json::ValueBuilder WriteMessage(const ::google::protobuf::Message& message, const PrintOptions& options) {
    ValueBuilderWriter writer;

    ProtoMessageVisitor visitor{writer};
    visitor.SetAlwaysPrintFieldsWithNoPresence(options.always_print_fields_with_no_presence);
    visitor.SetAlwaysPrintEnumsAsInts(options.always_print_enums_as_ints);
    visitor.SetPreserveProtoFieldNames(options.preserve_proto_field_names);
    visitor.SetExpandAny(!options.nonportable_raw_any);

    try {
        visitor(message);
    } catch (const FieldError& error) {
        throw PrintError(error.GetCode<PrintErrorCode>(), error.GetPath(), error.GetDescription());
    }

    return writer.ExtractResult();
}

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
