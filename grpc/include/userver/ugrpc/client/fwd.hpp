#pragma once

/// @file userver/ugrpc/client/fwd.hpp
/// @brief Forward declarations for `ugrpc::client` classes.

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename T>
class DefaultDict;

}  // namespace utils

namespace testsuite {
class GrpcControl;
}  // namespace testsuite

namespace ugrpc::client {

class ClientFactory;
class GenericClient;
struct Qos;
using ClientQos = utils::DefaultDict<Qos>;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
