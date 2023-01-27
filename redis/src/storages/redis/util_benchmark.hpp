#pragma once

#include <functional>

#include <benchmark/benchmark.h>

#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::bench {

class Redis : public benchmark::Fixture {
 protected:
  ClientPtr GetClient() const noexcept { return client_; };

  void RunStandalone(std::function<void()> payload);

  void RunStandalone(std::size_t thread_count, std::function<void()> payload);

 private:
  ClientPtr client_;
};

}  // namespace storages::redis::bench

USERVER_NAMESPACE_END
