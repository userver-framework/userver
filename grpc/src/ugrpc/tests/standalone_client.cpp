#include <userver/ugrpc/tests/standalone_client.hpp>

#include <fmt/format.h>

#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task_base.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

StandaloneClientFactory::StandaloneClientFactory(client::ClientFactorySettings&& client_factory_settings)
    : client_factory_{
          std::move(client_factory_settings),
          engine::current_task::GetTaskProcessor(),
          client::MiddlewareFactories{},
          completion_queues_,
          client_statistics_storage_,
          testsuite_control_,
          config_storage_.GetSource(),
      } {}

std::uint16_t GetFreeIpv6Port() {
    engine::io::Socket socket{engine::io::AddrDomain::kInet6, engine::io::SocketType::kStream};
    auto addr = engine::io::Sockaddr::MakeLoopbackAddress();
    addr.SetPort(0);
    socket.Bind(addr);
    return socket.Getsockname().Port();
}

std::string MakeIpv6Endpoint(std::uint16_t port) { return fmt::format("[::1]:{}", port); }

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
