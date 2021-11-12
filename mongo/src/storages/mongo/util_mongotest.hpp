#pragma once

#include <string>

#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

#include <storages/mongo/mongo_config.hpp>

USERVER_NAMESPACE_BEGIN

std::string GetTestsuiteMongoUri(const std::string& database);

clients::dns::Resolver MakeDnsResolver();

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, clients::dns::Resolver* dns_resolver,
    storages::mongo::Config mongo_config = {});

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config,
    clients::dns::Resolver* dns_resolver,
    storages::mongo::Config mongo_config = {});

USERVER_NAMESPACE_END
