#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Client;
using ClientPtr = std::shared_ptr<Client>;

}  // namespace storages::etcd

namespace ugrpc::client {

class ClientFactory;

}

USERVER_NAMESPACE_END

namespace etcdserverpb {

class KVClient;
using KVClientUPtr = std::unique_ptr<KVClient>;

class RangeResponse;

}  // namespace etcdserverpb

namespace mvccpb {

class KeyValue;

}  // namespace mvccpb