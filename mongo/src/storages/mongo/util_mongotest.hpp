#pragma once

#include <string>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

std::string GetTestsuiteMongoUri(const std::string& database);

storages::mongo::Pool MakeTestsuiteMongoPool(const std::string& name);

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config);
