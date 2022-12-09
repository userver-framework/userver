#pragma once

#include <string>

#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage_mock.hpp>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

std::string GetTestsuiteMongoUri(const std::string& database);

clients::dns::Resolver MakeDnsResolver();

dynamic_config::StorageMock MakeDynamicConfig();

dynamic_config::Source GetDefaultDynamicConfig();

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source = GetDefaultDynamicConfig());

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config,
    clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source = GetDefaultDynamicConfig());

USERVER_NAMESPACE_END
