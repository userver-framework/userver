#include <userver/storages/etcd/client.hpp>
#include <userver/components/component.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/storages/etcd/fwd.hpp>
#include <userver/storages/etcd/watch.hpp>

namespace etcdserverpb {

class RangeRequest;
class DeleteRangeRequest;

}  // namespace etcdserverpb

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class ClientImpl : public Client,
                   public std::enable_shared_from_this<ClientImpl> {
 public:
  ClientImpl(const components::ComponentConfig& config,
             const components::ComponentContext& context);

  void Put(const std::string& key, const std::string& value) const final;

  Response Get(const std::string& key_begin,
               const std::optional<std::string>& key_end) const final;

  Response GetByPrefix(const std::string& prefix) const final;

  void Delete(const std::string& key_begin,
              const std::optional<std::string>& key_end) const final;

  void DeleteByPrefix(const std::string& prefix) const final;

  void SetWatchCallback(std::function<void(bool, std::string, std::string)>) final;

  bool IsCallbackSet();

 private:
  Response MakeGet(const etcdserverpb::RangeRequest& request) const;
  void MakeDelete(const etcdserverpb::DeleteRangeRequest& request) const;

  ugrpc::client::ClientFactory& client_factory_;
  etcdserverpb::KVClientUPtr grpc_client_;
  WatchClient& watch_client_;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END