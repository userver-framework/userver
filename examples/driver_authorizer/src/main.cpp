#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include <utils/daemon_run.hpp>

#include "handlers/driver_session.hpp"
#include "taxi_config/taxi_config.hpp"

#include <yandex/taxi/userver/components/component_list.hpp>
#include <yandex/taxi/userver/components/thread_pool.hpp>
#include <yandex/taxi/userver/logging/component.hpp>
#include <yandex/taxi/userver/server/component.hpp>
#include <yandex/taxi/userver/server/handlers/ping.hpp>
#include <yandex/taxi/userver/server/handlers/server_monitor.hpp>
#include <yandex/taxi/userver/storages/mongo/component.hpp>
#include <yandex/taxi/userver/storages/redis/component.hpp>
#include <yandex/taxi/userver/storages/secdist/component.hpp>
#include <yandex/taxi/userver/taxi_config/component.hpp>
#include <yandex/taxi/userver/taxi_config/http_server_settings_component.hpp>

namespace driver_authorizer {

const auto kComponentList =
    components::ComponentList()
        .Append<components::Logging>()
        .Append<components::Secdist>()
        .Append<components::ThreadPool>("redis-thread-pool")
        .Append<components::ThreadPool>("redis-sentinel-thread-pool")
        .Append<components::Redis>()
        .Append<components::Mongo>("mongo-taxi")
        .Append<components::TaxiConfig>()
	.Append<components::HttpServerSettings>()
        .Append<components::Server>()
        .Append<handlers::DriverSession>()
        .Append<server::handlers::Ping>()
        .Append<server::handlers::ServerMonitor>();

}  // namespace driver_authorizer

int main(int argc, char** argv) {
  return utils::DaemonMain(argc, argv, driver_authorizer::kComponentList);
}
