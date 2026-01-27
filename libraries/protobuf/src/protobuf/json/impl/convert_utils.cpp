#include <protobuf/json/impl/convert_utils.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

MessageType ClassifyMessage(const std::string_view full_name) noexcept {
    // can't use simple dynamic_cast to determiine message types with special
    // handing because this will break support for dynamic messages
    constexpr std::string_view kProtobufPackage = "google.protobuf.";

    if (full_name.find(kProtobufPackage) != std::size_t{0}) {
        return MessageType::kGeneral;
    }

    auto name = full_name.substr(kProtobufPackage.size());

    switch (name[0]) {
        case 'A': {
            if (name == "Any") {
                return MessageType::kAny;
            }

            break;
        }
        case 'B': {
            if (name == "BoolValue") {
                return MessageType::kBoolValue;
            } else if (name == "BytesValue") {
                return MessageType::kBytesValue;
            }

            break;
        }
        case 'D': {
            if (name == "DoubleValue") {
                return MessageType::kDoubleValue;
            } else if (name == "Duration") {
                return MessageType::kDuration;
            }

            break;
        }
        case 'F': {
            if (name == "FieldMask") {
                return MessageType::kFieldMask;
            } else if (name == "FloatValue") {
                return MessageType::kFloatValue;
            }

            break;
        }
        case 'I': {
            if (name == "Int32Value") {
                return MessageType::kInt32Value;
            } else if (name == "Int64Value") {
                return MessageType::kInt64Value;
            }

            break;
        }
        case 'L': {
            if (name == "ListValue") {
                return MessageType::kListValue;
            }

            break;
        }
        case 'S': {
            if (name == "StringValue") {
                return MessageType::kStringValue;
            } else if (name == "Struct") {
                return MessageType::kStruct;
            }

            break;
        }
        case 'T': {
            if (name == "Timestamp") {
                return MessageType::kTimestamp;
            }

            break;
        }
        case 'U': {
            if (name == "UInt32Value") {
                return MessageType::kUInt32Value;
            } else if (name == "UInt64Value") {
                return MessageType::kUInt64Value;
            }

            break;
        }
        case 'V': {
            if (name == "Value") {
                return MessageType::kValue;
            }

            break;
        }
        default:
            break;
    }

    return MessageType::kGeneral;
}

const ::google::protobuf::FieldDescriptor& GetMessageFieldDesc(
    const ::google::protobuf::Descriptor& message_desc,
    const int field_number,
    const ::google::protobuf::FieldDescriptor::Type field_type,
    const bool is_repeated
) {
    const auto field_desc = message_desc.FindFieldByNumber(field_number);
    UINVARIANT(
        field_desc && field_desc->type() == field_type && field_desc->is_repeated() == is_repeated,
        fmt::format("Message type '{}' does not have expected layout", message_desc.full_name())
    );
    return *field_desc;
}

const ::google::protobuf::Descriptor* FindMessageDescByTypeUrl(
    const ::google::protobuf::DescriptorPool& pool,
    const std::string_view type_url
) noexcept {
    const auto pos = type_url.rfind('/');

    if (pos == std::string_view::npos || pos == 0) {
        return nullptr;
    }

    return pool.FindMessageTypeByName(type_url.substr(pos + 1));
}

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
