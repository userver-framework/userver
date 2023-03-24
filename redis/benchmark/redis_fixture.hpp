#pragma once

#include <functional>
#include <memory>

#include <benchmark/benchmark.h>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::bench {

using SentinelPtr = std::shared_ptr<USERVER_NAMESPACE::redis::Sentinel>;

class Redis : public benchmark::Fixture {
 protected:
  ClientPtr GetClient() const noexcept { return client_; };
  SentinelPtr GetSentinel() const noexcept { return sentinel_; };

  void RunStandalone(std::function<void()> payload);

 private:
  ClientPtr client_;
  SentinelPtr sentinel_;
};

}  // namespace storages::redis::bench

USERVER_NAMESPACE_END
