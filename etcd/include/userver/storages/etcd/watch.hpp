#pragma once

#include <memory>
#include <optional>
#include <userver/components/loggable_component_base.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/components/component.hpp>

#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/concurrent/background_task_storage.hpp>

#include <userver/utest/using_namespace_userver.hpp> 
#include <userver/dynamic_config/source.hpp>

#include <atomic>
#include "userver/engine/mutex.hpp"
#include "userver/engine/task/task.hpp"
#include "userver/engine/task/task_with_result.hpp"
#include "userver/storages/etcd/fwd.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {
 
class WatchClient final : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "watch-client";
 
  WatchClient(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  void Init();

  void Reset(/*std::string& key, std::string& range_end*/);

  void Destroy();

  void SetCallback(std::function<void(bool, std::string, std::string)> func);
 
  bool IsCallbackSet() const;

  ~WatchClient() final;
 
 private:
  userver::ugrpc::client::ClientFactory& grpc_client_factory_;
  std::shared_ptr<etcdserverpb::WatchClient> grpc_watch_client_;
  userver::engine::TaskWithResult<void> task_with_result_;
  std::optional<etcdserverpb::WatchClient::WatchCall> stream_;
  engine::Mutex stream_mutex_;

  int64_t watch_id = 0;
  int64_t create_cooldown;

  std::atomic<bool> to_stop;
  std::atomic<bool> to_reset;
  std::atomic<bool> watch_callback_set;
  std::atomic<bool> watch_start;

  std::function<void(bool, std::string, std::string)> watch_callback;
};
 
}  // namespace storages::etcd

USERVER_NAMESPACE_END
