#include <components/component_list.hpp>
#include <components/thread_pool.hpp>
#include <logging/component.hpp>
#include <server/component.hpp>
#include <server/handlers/ping.hpp>
#include <server/handlers/server_monitor.hpp>
#include <storages/mongo/component.hpp>
#include <storages/redis/component.hpp>
#include <storages/secdist/component.hpp>
#include <taxi_config/component.hpp>
#include <taxi_config/need_log_request_checker_component.hpp>
#include <utils/daemon_run.hpp>

#include "handlers/driver_session.hpp"
#include "taxi_config/taxi_config.hpp"

namespace driver_authorizer {

const auto kComponentList =
    components::ComponentList()
        .Append<components::Logging>()
        .Append<components::Secdist>()
        .Append<components::ThreadPool>("redis-thread-pool")
        .Append<components::ThreadPool>("redis-sentinel-thread-pool")
        .Append<components::Redis>()
        .Append<components::Mongo>("mongo-taxi")
        .Append<components::TaxiConfig<TaxiConfig>>()
        .Append<components::NeedLogRequestChecker<TaxiConfig>>()
        .Append<components::Server>()
        .Append<handlers::DriverSession>()
        .Append<server::handlers::Ping>()
        .Append<server::handlers::ServerMonitor>();

}  // namespace driver_authorizer

int main(int argc, char** argv) {
  return utils::DaemonMain(argc, argv, driver_authorizer::kComponentList);
}
