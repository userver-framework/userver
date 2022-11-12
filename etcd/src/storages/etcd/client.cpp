#include <userver/storages/etcd/client.hpp>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

  void Client::Remove(const std::string& /*key*/)
  {
    // throw std::runtime_error("Not implemented");
  }

}  // namespace storages::etcd

USERVER_NAMESPACE_END
