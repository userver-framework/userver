#include "client_impl.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <userver/components/component.hpp>
#include <userver/storages/etcd/client_fwd.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/storages/etcd/response_etcd.hpp>
#include <etcd/api/etcdserverpb/etcdserver.pb.h>
#include <etcd/api/etcdserverpb/rpc.pb.h>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

ClientImpl::ClientImpl(etcdserverpb::KVClientUPtr grpc_client)
    : grpc_client_(std::move(grpc_client)) {}

  Response ClientImpl::Get(const std::string& key) const {
    etcdserverpb::RangeRequest request;
    request.set_key(key);

    return MakeGet(request);
  }

  Response ClientImpl::GetRange(const std::string& key_begin, const std::string& key_end) const
  {
    etcdserverpb::RangeRequest request;
    request.set_key(key_begin);
    request.set_range_end(key_end);

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

    auto* mutalbe_kvs = response.mutable_kvs();
    std::vector<KeyValue> results;
    results.reserve(mutalbe_kvs->size());
    for (auto kv : *mutalbe_kvs) {
      results.emplace_back(*kv.mutable_key(), *kv.mutable_value());
    }

    return Response{results};
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

  void ClientImpl::Delete(const std::string &key) const {
    etcdserverpb::DeleteRangeRequest request;
    request.set_key(key);
    
    MakeDelete(request);
  }

  void ClientImpl::DeleteRange(const std::string &key_begin, const std::string &key_end) const {
    etcdserverpb::DeleteRangeRequest request;
    request.set_key(key_begin);
    request.set_range_end(key_end);

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

}  // namespace storages::etcd

USERVER_NAMESPACE_END
