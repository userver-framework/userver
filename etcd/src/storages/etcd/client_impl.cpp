#include "client_impl.hpp"
#include <stdexcept>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

  Request ClientImpl::GetRange(const std::string& /*key_begin*/, const std::string& /*key_end*/)
  {
    return Request{};
    // throw std::runtime_error("Not implemented");
  }

}  // namespace storages::etcd

USERVER_NAMESPACE_END
