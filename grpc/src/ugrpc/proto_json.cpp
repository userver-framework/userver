#include <userver/ugrpc/proto_json.hpp>

#include <cstddef>

#include <fmt/format.h>
#include <grpcpp/support/config.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace impl {

const google::protobuf::util::JsonPrintOptions kDefaultJsonPrintOptions = [] {
    google::protobuf::util::JsonPrintOptions options;
#if GOOGLE_PROTOBUF_VERSION >= 5026001
    options.always_print_fields_with_no_presence = true;
#else
    options.always_print_primitive_fields = true;
#endif
    return options;
}();

const google::protobuf::util::JsonParseOptions kDefaultJsonParseOptions = [] {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = false;
    options.case_insensitive_enum_parsing = false;
    return options;
}();

void FromJsonStringImpl(
    std::string_view json_string,
    google::protobuf::Message& output,
    const google::protobuf::util::JsonParseOptions& options
) {
#if defined(ARCADIA_ROOT)
    // JSON utils use y_absl::string_view.
    const auto status = google::protobuf::util::JsonStringToMessage(
        y_absl::string_view(json_string.data(), json_string.size()),
        &output,
        options
    );
#elif GOOGLE_PROTOBUF_VERSION >= 4022000
    // JSON utils use absl::string_view.
    const auto status = google::protobuf::util::JsonStringToMessage(
        absl::string_view(json_string.data(), json_string.size()),
        &output,
        options
    );
#else
    // JSON utils use StringPiece.
    const auto status = google::protobuf::util::JsonStringToMessage(
        google::protobuf::StringPiece(json_string.data(), json_string.size()),
        &output,
        options
    );
#endif

    if (!status.ok()) {
#if GOOGLE_PROTOBUF_VERSION >= 4022000
        // JSON utils use absl::string_view.
        const std::string_view message(status.message().data(), status.message().size());
#else
        // JSON utils use StringPiece.
        const std::string_view message(status.message().data(), static_cast<std::size_t>(status.message().size()));
#endif
        throw formats::json::Exception(fmt::format("Cannot parse protobuf from string: {}", message));
    }
}

}  // namespace impl

formats::json::Value MessageToJson(const google::protobuf::Message& message) {
    return MessageToJson(message, impl::kDefaultJsonPrintOptions);
}

formats::json::Value MessageToJson(
    const google::protobuf::Message& message,
    const google::protobuf::util::JsonPrintOptions& options
) {
    return formats::json::FromString(ToJsonString(message, options));
}

std::string ToJsonString(const google::protobuf::Message& message) {
    return ToJsonString(message, impl::kDefaultJsonPrintOptions);
}

std::string ToJsonString(
    const google::protobuf::Message& message,
    const google::protobuf::util::JsonPrintOptions& options
) {
    grpc::string result{};

    auto status = google::protobuf::util::MessageToJsonString(message, &result, options);

    if (!status.ok()) {
        throw formats::json::Exception("Cannot convert protobuf to string");
    }

    return result;
}

}  // namespace ugrpc

namespace formats::serialize {

json::Value Serialize(const google::protobuf::Message& message, To<json::Value>) {
    return ugrpc::MessageToJson(message);
}

}  // namespace formats::serialize

namespace formats::parse {
namespace {

enum class Type { kStruct, kArray };

Type ParseType(const formats::json::Value& value) { return value.IsObject() ? Type::kStruct : Type::kArray; }

Type ParseType(const formats::json::Value::const_iterator& iter) {
    return iter->IsObject() ? Type::kStruct : Type::kArray;
}

std::string GetName(const formats::json::Value::const_iterator& iter, const Type previous_type) {
    return previous_type == Type::kStruct ? iter.GetName() : "";
}

class ResultStackFrame final {
public:
    explicit ResultStackFrame(const formats::json::Value& value)
        : ResultStackFrame(ParseType(value), value.GetSize(), "")
    {}

    ResultStackFrame(const formats::json::Value::const_iterator& iter, const Type previous_type)
        : ResultStackFrame(ParseType(iter), iter->GetSize(), GetName(iter, previous_type))
    {}

    void SetStructField(std::string_view field_name, google::protobuf::Value&& field) {
        UINVARIANT(type_ == Type::kStruct, "invalid type");
#if GOOGLE_PROTOBUF_VERSION >= 4022000
        (*value_.mutable_struct_value()->mutable_fields())[field_name] = std::move(field);
#else
        // No transparent comparisons till
        // https://github.com/protocolbuffers/protobuf/commit/38d6de1eef8163342084fe
        (*value_.mutable_struct_value()->mutable_fields())[std::string{field_name}] = std::move(field);
#endif

        --elements_await_;
    }

    void AddListElement(google::protobuf::Value&& field) {
        UINVARIANT(type_ == Type::kArray, "invalid type");
        *(value_.mutable_list_value()->mutable_values()->Add()) = std::move(field);
        --elements_await_;
    }

    bool IsStruct() const { return type_ == Type::kStruct; }

    bool IsArray() const { return type_ == Type::kArray; }

    Type GetType() const { return type_; }

    bool AwaitElements() const { return elements_await_ != 0; }

    std::string_view GetOuterFieldName() const { return outer_field_name_; }

    google::protobuf::Value GetValue() { return google::protobuf::Value(std::move(value_)); }

private:
    ResultStackFrame(const Type type, std::size_t elements_await, std::string&& outer_field_name)
        : type_(type),
          elements_await_(elements_await),
          outer_field_name_(outer_field_name)
    {
        if (type == Type::kStruct) {
            value_.mutable_struct_value();
        } else {
            value_.mutable_list_value();
        }
    }

private:
    Type type_;
    std::size_t elements_await_;
    std::string outer_field_name_;
    google::protobuf::Value value_{};
};

class StackFrame {
public:
    using Iterator = formats::json::Value::const_iterator;

    StackFrame(Iterator begin, Iterator end)
        : cur_(begin),
          end_(end)
    {}

    bool IsTrivial() const { return !(cur_->IsObject() || cur_->IsArray()); }

    std::size_t GetSize() const { return cur_->GetSize(); }

    Iterator GetIter() const { return cur_; }

    std::string GetName() const { return cur_.GetName(); }

    void Advance() { ++cur_; }

    bool IsValid() const { return cur_ != end_; }

private:
    Iterator cur_;
    Iterator end_;
};

static constexpr std::size_t kInitialStackDepth = 32;

template <typename T>
using Stack = boost::container::small_vector<T, kInitialStackDepth>;

google::protobuf::Value ParseValue(const formats::json::Value& value) {
    google::protobuf::Value result;
    if (value.IsDouble()) {
        result.set_number_value(value.As<double>());
    } else if (value.IsInt64()) {
        result.set_number_value(value.As<std::int64_t>());
    } else if (value.IsUInt64()) {
        result.set_number_value(value.As<std::uint64_t>());
    } else if (value.IsBool()) {
        result.set_bool_value(value.As<bool>());
    } else if (value.IsString()) {
        result.set_string_value(value.As<std::string>());
    } else if (value.IsNull()) {
        result.set_null_value(google::protobuf::NullValue{});
    } else {
        throw formats::json::ParseException("unsupported type");
    }
    return result;
}

google::protobuf::Value ParseImpl(const formats::json::Value& value) {
    if (!(value.IsArray() || value.IsObject())) {
        return ParseValue(value);
    }
    if (value.IsEmpty()) {
        return ResultStackFrame(value).GetValue();
    }

    Stack<StackFrame> stack{};
    Stack<ResultStackFrame> result(1, ResultStackFrame(value));

    stack.emplace_back(value.begin(), value.end());
    for (;;) {
        if (stack.back().IsTrivial()) {
            if (result.back().IsStruct()) {
                result.back().SetStructField(stack.back().GetName(), ParseValue(*stack.back().GetIter()));
            } else {
                result.back().AddListElement(ParseValue(*stack.back().GetIter()));
            }
            stack.back().Advance();
        } else {
            result.emplace_back(stack.back().GetIter(), result.back().GetType());
            auto stack_back_value = stack.back().GetIter();
            stack.back().Advance();
            stack.emplace_back(stack_back_value->begin(), stack_back_value->end());
        }

        while (result.size() >= 2 && !result.back().AwaitElements()) {
            auto prev_back = result.end() - 2;
            auto back = result.back();
            if (prev_back->AwaitElements()) {
                if (prev_back->IsStruct()) {
                    prev_back->SetStructField(back.GetOuterFieldName(), back.GetValue());
                } else {
                    prev_back->AddListElement(back.GetValue());
                }
                result.pop_back();
            }
        }
        while (!stack.back().IsValid()) {
            stack.pop_back();
            if (stack.empty()) {
                return result.back().GetValue();
            }
        }
    }
}

}  // namespace

// TODO use iterative implementation for any google::protobuf::Message, not just for top-level Value and Struct.
google::protobuf::Value Parse(const json::Value& value, To<google::protobuf::Value>) { return ParseImpl(value); }

google::protobuf::Struct Parse(const json::Value& value, To<google::protobuf::Struct>) {
    value.CheckObject();
    auto protobuf_value = ParseImpl(value);
    UASSERT(protobuf_value.has_struct_value());
    return std::move(*protobuf_value.mutable_struct_value());
}

google::protobuf::ListValue Parse(const json::Value& value, To<google::protobuf::ListValue>) {
    value.CheckArray();
    auto protobuf_value = ParseImpl(value);
    UASSERT(protobuf_value.has_list_value());
    return std::move(*protobuf_value.mutable_list_value());
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
