#include "client_impl.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <userver/components/component.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/storages/etcd/response.hpp>
#include "userver/storages/etcd/watch.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

<<<<<<< HEAD
ClientImpl::ClientImpl(etcdserverpb::KVClientUPtr grpc_client)
    : grpc_client_(std::move(grpc_client)) {}
=======
ClientImpl::ClientImpl(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
    : client_factory_(
          context.FindComponent<ugrpc::client::ClientFactoryComponent>()
              .GetFactory()),
      grpc_client_(std::make_unique<etcdserverpb::KVClient>(
          client_factory_.MakeClient<etcdserverpb::KVClient>(
              config["endpoint"].As<std::string>()))),
      watch_client_(context.FindComponent<WatchClient>("watch-client")) {}
>>>>>>> 182b2651 (moving watch to etcd)

Response ClientImpl::Get(const std::string& key_begin,
                         const std::optional<std::string>& key_end) const {
  etcdserverpb::RangeRequest request;
  request.set_key(key_begin);
  if (key_end.has_value()) {
    request.set_range_end(*key_end);
  }

  return MakeGet(request);
}

Response ClientImpl::GetByPrefix(const std::string &prefix) const {
  etcdserverpb::RangeRequest request;
  request.set_key(prefix);
  if (!prefix.empty()) {
    auto prefix_end = prefix;
    ++prefix_end.back();
    request.set_range_end(prefix_end);
  }

  return MakeGet(request);
}

Response ClientImpl::MakeGet(const etcdserverpb::RangeRequest& request) const {
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

void ClientImpl::Delete(const std::string &key_begin,
                        const std::optional<std::string>& key_end) const {
  etcdserverpb::DeleteRangeRequest request;
  request.set_key(key_begin);
  if (key_end.has_value()) {
    request.set_range_end(*key_end);
  }

  MakeDelete(request);
}

void ClientImpl::DeleteByPrefix(const std::string &prefix) const {
  etcdserverpb::DeleteRangeRequest request;
  request.set_key(prefix);
  if (!prefix.empty()) {
    auto prefix_end = prefix;
    ++prefix_end.back();
    request.set_range_end(prefix_end);
  }

  MakeDelete(request);
}

void ClientImpl::MakeDelete(const etcdserverpb::DeleteRangeRequest& request) const {
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));
  auto stream = grpc_client_->DeleteRange(request, std::move(context));
  etcdserverpb::DeleteRangeResponse response = stream.Finish();
}

void ClientImpl::SetWatchCallback(std::function<void(bool, std::string, std::string)> func){
  watch_client_.SetCallback(std::move(func));
}

}  // namespace storages::etcd

USERVER_NAMESPACE_END
