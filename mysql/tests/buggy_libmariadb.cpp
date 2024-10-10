#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/task.hpp>

#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/settings/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

UTEST(Libmariadb, CONC622) {
  clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(), {}};

  const settings::EndpointInfo endpoint_info{"localhost", 65535};
  const settings::AuthSettings auth_settings{};  // doesn't matter
  const settings::ConnectionSettings connection_settings{
      1, false, false, settings::IpMode::kIpV4};

  const auto try_connect = [&] {
    return impl::Connection{
        resolver, endpoint_info, auth_settings, connection_settings,
        engine::Deadline::FromDuration(std::chrono::milliseconds{200})};
  };

  EXPECT_THROW(try_connect(), MySQLException);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
