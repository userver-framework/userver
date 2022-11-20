#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <userver/storages/etcd/client.hpp>
#include <userver/storages/etcd/client_fwd.hpp>
#include "userver/storages/etcd/response_etcd.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class ClientImpl : public Client,
                    public std::enable_shared_from_this<ClientImpl>
{
public:
    ClientImpl(etcdserverpb::KVClientUPtr grpc_client);
    Response Get(const std::string& key) const final;
    Response GetRange(const std::string& key_begin, const std::string& key_end) const final;
    Response GetByPrefix(const std::string& prefix) const final;
    void Put(const std::string& key, const std::string& value) const final;
    void Delete(const std::string& key) const final;
    void DeleteRange(const std::string& key_begin, const std::string& key_end) const final;
    void DeleteByPrefix(const std::string& prefix) const final;

private:
    Response MakeGet(const etcdserverpb::RangeRequest& request) const;
    void MakeDelete(const etcdserverpb::DeleteRangeRequest& request) const;

    etcdserverpb::KVClientUPtr grpc_client_;
};

}  // namespace storages::etcd

USERVER_NAMESPACE_END