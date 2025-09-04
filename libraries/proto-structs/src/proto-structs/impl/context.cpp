#include <userver/proto-structs/impl/context.hpp>

#include <google/protobuf/descriptor.h>

#include <userver/utils/assert.hpp>

namespace proto_structs::impl {

const ConversionError& Context::GetError() const& {
    UINVARIANT(HasError(), "Context does not contain an error");
    return error_.value();
}

ConversionError&& Context::GetError() && {
    UINVARIANT(HasError(), "Context does not contain an error");
    return std::move(error_).value();
}

void Context::SetError(const ::google::protobuf::FieldDescriptor* field_desc, std::string_view reason) {
    error_.emplace(field_desc->containing_type()->full_name(), field_desc->name(), reason);
}

}  // namespace proto_structs::impl
