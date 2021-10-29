#pragma once

#include <string>

#include <userver/clients/dns/resolver.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

std::string GetTestsuiteMongoUri(const std::string& database);

clients::dns::Resolver MakeDnsResolver();

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, clients::dns::Resolver* dns_resolver);

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config,
    clients::dns::Resolver* dns_resolver);

USERVER_NAMESPACE_END
