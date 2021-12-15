#pragma once

#include <userver/storages/redis/impl/secdist_redis.hpp>

USERVER_NAMESPACE_BEGIN

const secdist::RedisSettings& GetTestsuiteRedisSettings();

const secdist::RedisSettings& GetTestsuiteRedisClusterSettings();

USERVER_NAMESPACE_END
