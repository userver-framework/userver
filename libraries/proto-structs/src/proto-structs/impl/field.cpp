#include <userver/proto-structs/impl/field.hpp>

#include <google/protobuf/message.h>

#include <userver/utils/assert.hpp>

namespace proto_structs::impl {

const ::google::protobuf::FieldDescriptor* FieldAccessor::GetFieldDescriptor() const noexcept {
    auto result = message_.GetDescriptor()->FindFieldByNumber(field_number_);
    UASSERT_MSG(result, "Field #" + std::to_string(field_number_) + " descriptor is not found");
    return result;
}

}  // namespace proto_structs::impl
