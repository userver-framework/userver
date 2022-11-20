#include "client_impl.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "userver/storages/etcd/response.hpp"

#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <userver/components/component.hpp>
#include <userver/storages/etcd/client_fwd.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/storages/etcd/response.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

ClientImpl::ClientImpl(etcdserverpb::KVClientUPtr grpc_client)
    : grpc_client_(std::move(grpc_client)) {}

Response ClientImpl::GetRange(const std::string& key_begin,
                             const std::string& /*key_end*/) const {
  etcdserverpb::RangeRequest request;
  request.set_key(key_begin);
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));
  auto stream = grpc_client_->Range(request, std::move(context));
  etcdserverpb::RangeResponse response = stream.Finish();

  return Response(std::move(response));
}

void ClientImpl::Put(const std::string& key, const std::string& value) const {
  etcdserverpb::PutRequest request;
  request.set_key(key);
  request.set_value(value);
  request.set_lease(0);
  request.set_prev_kv(false);
  request.set_ignore_value(false);
  request.set_ignore_lease(false);

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));
  auto stream = grpc_client_->Put(request, std::move(context));

  etcdserverpb::PutResponse response = stream.Finish();
}

void ClientImpl::Delete(const std::string& key) const {
  etcdserverpb::DeleteRangeRequest request;
  request.set_key(key);
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));
  auto stream = grpc_client_->DeleteRange(request, std::move(context));
  etcdserverpb::DeleteRangeResponse response = stream.Finish();
}

}  // namespace storages::etcd

USERVER_NAMESPACE_END
