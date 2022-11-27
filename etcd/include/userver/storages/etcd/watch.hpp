#pragma once

#include <memory>
#include <userver/components/loggable_component_base.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/components/component.hpp>

#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/concurrent/background_task_storage.hpp>

#include <userver/utest/using_namespace_userver.hpp> 
#include <userver/dynamic_config/source.hpp>

#include <atomic>
#include "userver/storages/etcd/fwd.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {
 
class WatchClient final : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "watch-client";
 
  WatchClient(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  userver::ugrpc::client::BidirectionalStream<etcdserverpb::WatchRequest, etcdserverpb::WatchResponse> Init();

  void Reset(/*std::string& key, std::string& range_end*/);

  void Destroy(userver::ugrpc::client::BidirectionalStream<etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>& stream);

  void SetCallback(std::function<void(bool, std::string, std::string)> func);
 
  bool IsCallbackSet() const;

  ~WatchClient() final;
 
 private:
  userver::ugrpc::client::ClientFactory& grpc_client_factory_;
  std::shared_ptr<etcdserverpb::WatchClient> grpc_watch_client_;
  userver::concurrent::BackgroundTaskStorage bts;
  int64_t watch_id;
  int64_t create_cooldown;

  std::atomic<bool> to_stop;
  std::atomic<bool> to_reset;
  std::atomic<bool> watch_callback_set;
  std::atomic<bool> watch_start;

  std::function<void(bool, std::string, std::string)> watch_callback;
};
 
}  // namespace storages::etcd

USERVER_NAMESPACE_END
