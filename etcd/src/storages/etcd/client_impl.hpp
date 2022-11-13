#include <userver/storages/etcd/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd
{

class ClientImpl : public Client,
                    public std::enable_shared_from_this<ClientImpl>
{
public:
    Request GetRange(const std::string& key_begin, const std::string& key_end) final;
};

} // namespace storages::etcd

USERVER_NAMESPACE_END