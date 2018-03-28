#pragma once

#include <components/thread_pool.hpp>
#include <logging/component.hpp>
#include <server/component_list.hpp>
#include <server/handlers/ping.hpp>
#include <storages/mongo/component.hpp>
#include <storages/redis/component.hpp>
#include <storages/secdist/component.hpp>
#include <taxi_config/component.hpp>

#include "handlers/driver_session.hpp"

namespace driver_authorizer {

const auto kComponentList =
    server::ComponentList()
        .Append<components::Logging>()
        .Append<components::Secdist>()
        .Append<components::ThreadPool>("redis-thread-pool")
        .Append<components::ThreadPool>("redis-sentinel-thread-pool")
        .Append<components::Redis>()
        .Append<components::Mongo>("mongo-taxi")
        .Append<components::TaxiConfig>()
        .Append<handlers::DriverSession>()
        .Append<server::handlers::Ping>();

}  // namespace driver_authorizer
