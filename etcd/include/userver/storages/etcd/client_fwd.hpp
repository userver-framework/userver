#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Client;
using ClientPtr = std::shared_ptr<Client>;

}  // namespace storages::etcd

USERVER_NAMESPACE_END
