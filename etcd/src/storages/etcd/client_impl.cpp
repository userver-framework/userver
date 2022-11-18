#include "client_impl.hpp"
#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <stdexcept>
#include <string>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/components/component.hpp>
#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <userver/storages/etcd/client_fwd.hpp>
#include <vector>
#include "userver/storages/etcd/request.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

  ClientImpl::ClientImpl(etcdserverpb::KVClientUPtr grpc_client) :
    grpc_client_(std::move(grpc_client))
  {}

  Request ClientImpl::GetRange(const std::string& /*key_begin*/, const std::string& /*key_end*/)
  {
    return Request{};
  }

}  // namespace storages::etcd

USERVER_NAMESPACE_END
