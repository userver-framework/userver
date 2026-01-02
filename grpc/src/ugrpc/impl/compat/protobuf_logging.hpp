#pragma once

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::compat {

void Print(const google::protobuf::Message& message, google::protobuf::io::ZeroCopyOutputStream& output_stream);

}  // namespace ugrpc::impl::compat

USERVER_NAMESPACE_END
