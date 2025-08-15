#pragma once

#include <optional>
#include <string_view>

#include <userver/proto-structs/exceptions.hpp>

namespace google::protobuf {
class FieldDescriptor;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

class Context {
public:
    constexpr Context() noexcept = default;

    constexpr bool HasError() const noexcept { return error_.has_value(); }

    const ConversionError& GetError() const&;
    ConversionError&& GetError() &&;

    void SetError(const ::google::protobuf::FieldDescriptor* field_desc, std::string_view reason);

private:
    std::optional<ConversionError> error_;
};

class ReadContext final : public Context {
public:
    using Context::Context;
};

class WriteContext final : public Context {
public:
    using Context::Context;
};

}  // namespace proto_structs::impl

USERVER_NAMESPACE_END
