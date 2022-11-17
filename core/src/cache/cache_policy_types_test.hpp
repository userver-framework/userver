#pragma once

#include <userver/utest/utest.hpp>

#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

using PolicyTypes = ::testing::Types<
    std::integral_constant<cache::CachePolicy, cache::CachePolicy::kLRU>,
    std::integral_constant<cache::CachePolicy, cache::CachePolicy::kSLRU>,
    std::integral_constant<cache::CachePolicy, cache::CachePolicy::kTinyLFU>>;

USERVER_NAMESPACE_END
