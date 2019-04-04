#pragma once

#include <mutex>
#include <queue>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/request.hpp>
#include <clients/http/statistics.hpp>

namespace curl {
class multi;
}  // namespace curl

namespace engine {
namespace ev {

class ThreadPool;
}  // namespace ev
}  // namespace engine

namespace clients {
namespace http {

class EasyWrapper;

class Client {
 public:
  /* Use this method to create Client */
  static std::shared_ptr<Client> Create(size_t io_threads);

  Client(const Client&) = delete;
  Client(Client&&) = delete;
  ~Client();

  std::shared_ptr<Request> CreateRequest();

  void SetMaxPipelineLength(size_t max_pipeline_length);
  void SetMaxHostConnections(size_t max_host_connections);
  void SetConnectionPoolSize(size_t connection_pool_size);

  PoolStatistics GetPoolStatistics() const;

  /* Set max number of automatically created destination metrics */
  void SetDestinationMetricsAutoMaxSize(size_t max_size);

  DestinationStatistics::DestinationsMap GetDestinationStatistics() const;

 private:
  explicit Client(size_t io_threads);

  InstanceStatistics GetMultiStatistics(size_t n) const;

  size_t FindMultiIndex(const curl::multi&) const;

  // Functions for EasyWrapper that must be noexcept, as they are called from
  // the EasyWrapper destructor.
  friend class EasyWrapper;
  void IncPending() noexcept { ++pending_tasks_; }
  void DecPending() noexcept { --pending_tasks_; }
  void PushIdleEasy(std::unique_ptr<curl::easy> easy) noexcept;

  std::atomic<std::size_t> pending_tasks_{0};

  std::shared_ptr<DestinationStatistics> destination_statistics_;
  std::unique_ptr<engine::ev::ThreadPool> thread_pool_;
  std::vector<Statistics> statistics_;
  std::vector<std::unique_ptr<curl::multi>> multis_;

  std::mutex idle_easy_queue_mutex_;
  std::queue<std::unique_ptr<curl::easy>> idle_easy_queue_;
};

}  // namespace http
}  // namespace clients
