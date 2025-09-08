#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

const std::string kBodyTag{"body"};
const std::string kCodeTag{"grpc_code"};
const std::string kComponentTag{"grpc_component"};
const std::string kMessageMarshalledLenTag{"grpc_message_marshalled_len"};
const std::string kTypeTag{"grpc_type"};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
