#pragma once

/// @file userver/ugrpc/client/fwd.hpp
/// @brief Forward declarations for `ugrpc::client` classes.

USERVER_NAMESPACE_BEGIN

namespace testsuite {
class GrpcControl;
}  // namespace testsuite

namespace ugrpc::client {

class ClientFactory;
class GenericClient;
struct Qos;
struct ClientQos;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
