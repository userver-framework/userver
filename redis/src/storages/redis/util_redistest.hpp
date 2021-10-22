#pragma once

#include <userver/storages/redis/impl/secdist_redis.hpp>

USERVER_NAMESPACE_BEGIN

const secdist::RedisSettings& GetTestsuiteRedisSettings();

USERVER_NAMESPACE_END
