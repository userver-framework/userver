#include <userver/storages/etcd/client.hpp>
#include <userver/storages/etcd/client_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd
{

class ClientImpl : public Client,
                    public std::enable_shared_from_this<ClientImpl>
{
public:
    ClientImpl(etcdserverpb::KVClientUPtr grpc_client);
    Request GetRange(const std::string& key_begin, const std::string& key_end) const final;

private:
    etcdserverpb::KVClientUPtr grpc_client_;
};

} // namespace storages::etcd

USERVER_NAMESPACE_END