#include "client_impl.hpp"

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/urabbitmq/client_settings.hpp>

#include <urabbitmq/connection_pool.hpp>
#include <urabbitmq/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ClientImpl::ClientImpl(clients::dns::Resolver& resolver,
                       const ClientSettings& settings)
    : settings_{settings} {
  const auto endpoints_count = settings_.endpoints.endpoints.size();
  pools_.assign(endpoints_count, {{}, nullptr});

  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(endpoints_count);
  for (size_t i = 0; i < endpoints_count; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this, &resolver, i] {
      pools_[i].pool = ConnectionPool::Create(
          resolver, settings_.endpoints.endpoints[i], settings_.endpoints.auth,
          settings_.pool_settings, settings_.use_secure_connection,
          pools_[i].stats);
    }));
  }
  engine::WaitAllChecked(init_tasks);
}

void ClientImpl::WriteStatistics(utils::statistics::Writer& writer) const {
  for (size_t i = 0; i < settings_.endpoints.endpoints.size(); ++i) {
    writer[settings_.endpoints.endpoints[i].host] = pools_[i].stats.Get();
  }
}

ConnectionPtr ClientImpl::GetConnection(engine::Deadline deadline) {
  const auto start_idx = pool_idx_.load(std::memory_order_relaxed);
  for (size_t i = 0; i < pools_.size(); ++i) {
    const auto idx = (start_idx + i) % pools_.size();
    try {
      auto conn_ptr = pools_[idx].pool->Acquire(deadline);
      pool_idx_.fetch_add(i + 1, std::memory_order_relaxed);
      return conn_ptr;
    } catch (const std::exception&) {
    }
  }

  throw std::runtime_error{"No available connections in cluster"};
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
