#pragma once

#include <storages/clickhouse/impl/wrap_clickhouse_cpp.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/storages/clickhouse/execution_result.hpp>
#include <userver/storages/clickhouse/options.hpp>

#include <storages/clickhouse/impl/native_client_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {
class Query;
}

namespace storages::clickhouse::impl {

struct EndpointSettings;
struct AuthSettings;
struct ConnectionSettings;
class InsertionRequest;

class Connection final {
public:
    Connection(clients::dns::Resolver&, const EndpointSettings&, const AuthSettings&, const ConnectionSettings&);

    ExecutionResult Execute(OptionalCommandControl, const Query&);

    void Insert(OptionalCommandControl, const InsertionRequest&);

    void Ping();

    bool IsBroken() const noexcept;

private:
    class ConnectionBrokenGuard;
    ConnectionBrokenGuard GetBrokenGuard();

    void DoExecute(OptionalCommandControl, const clickhouse_cpp::Query&);

    NativeClientWrapper client_;
    bool broken_{false};
};

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
