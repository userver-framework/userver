#include <userver/proto-structs/io/impl/field_accessor.hpp>

#include <google/protobuf/message.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io::impl {

const ::google::protobuf::FieldDescriptor& FieldAccessor::GetFieldDescriptor() const noexcept {
    if (field_desc_) {
        return *field_desc_;
    }

    field_desc_ = message_.GetDescriptor()->FindFieldByNumber(field_number_);
    UASSERT(field_desc_);
    return *field_desc_;
}

}  // namespace proto_structs::io::impl

USERVER_NAMESPACE_END
