#pragma once

#include <userver/field_options.pb.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

inline auto GetFieldOptions(const google::protobuf::FieldDescriptor& field) {
  return field.options().GetExtension(userver::field);
}

void TrimSecrets(google::protobuf::Message& message);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
