#include "client_impl.hpp"
#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <stdexcept>
#include <string>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/components/component.hpp>
#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <userver/storages/etcd/client_fwd.hpp>
#include <utility>
#include <vector>
#include "userver/storages/etcd/request.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

  ClientImpl::ClientImpl(etcdserverpb::KVClientUPtr grpc_client) :
    grpc_client_(std::move(grpc_client))
  {}

  Request ClientImpl::GetRange(const std::string& key_begin, const std::string& /*key_end*/) const
  {
    etcdserverpb::RangeRequest request;
    request.set_key(std::move(key_begin));
    auto context = std::make_unique<grpc::ClientContext>();
    context->set_deadline(
        userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));
    auto stream = grpc_client_->Range(request, std::move(context));
    etcdserverpb::RangeResponse response = stream.Finish();

    auto* mutalbe_kvs = response.mutable_kvs();
    std::vector<Component> results;
    results.reserve(mutalbe_kvs->size());
    for (auto kv : *mutalbe_kvs)
    {
      results.emplace_back(*kv.mutable_key(), *kv.mutable_value());
    }

    return Request{std::move(results)};
  }

}  // namespace storages::etcd

USERVER_NAMESPACE_END
