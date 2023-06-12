#pragma once

#include <storages/redis/client_redistest.hpp>

USERVER_NAMESPACE_BEGIN

// This class is for better naming :) Use this fixture for tests
// for functionality that is valid only for some settings.
class RedisPubsubTest : public RedisClientTest {};

// Basic functionality should work for every possible CommandControl object
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
struct RedisPubsubTestBasic
    : public RedisPubsubTest,
      public ::testing::WithParamInterface<redis::CommandControl> {};

USERVER_NAMESPACE_END
