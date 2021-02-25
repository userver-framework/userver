#pragma once

/// @file clients/http/cleint.hpp
/// @brief @copybrief clients::http::Client

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use clients::Http from clients/http.hpp instead
#endif

#include <memory>

#include <moodycamel/concurrentqueue_fwd.h>

#include <clients/http/request.hpp>
#include <clients/http/statistics.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <rcu/rcu.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/periodic_task.hpp>
#include <utils/swappingsmart.hpp>

namespace curl {
class easy;
class multi;
class ConnectRateLimiter;
}  // namespace curl

namespace engine::ev {
class ThreadPool;
}  // namespace engine::ev

namespace clients::http {
namespace impl {
class EasyWrapper;
}  // namespace impl

class DestinationStatistics;
struct Config;
struct TestsuiteConfig;

/// @ingroup userver_clients
///
/// @brief HTTP client that returns a HTTP request builder from
/// CreateRequest().
///
/// ## Example usage:
///
/// @sample clients/http/client_test.cpp  Sample HTTP Client usage
class Client final {
 public:
  Client(const std::string& thread_name_prefix, size_t io_threads,
         engine::TaskProcessor& fs_task_processor);
  ~Client();

  /// @brief Returns a HTTP request builder type with preset values of
  /// User-Agent, Proxy and some of the Testsuite suff (if any).
  std::shared_ptr<Request> CreateRequest();

  /// Providing CreateNonSignedRequest() function for the clients::Http alias.
  std::shared_ptr<Request> CreateNotSignedRequest() { return CreateRequest(); }

  void SetMultiplexingEnabled(bool enabled);
  void SetMaxHostConnections(size_t max_host_connections);

  PoolStatistics GetPoolStatistics() const;

  /// @brief Set max number of automatically created destination metrics
  void SetDestinationMetricsAutoMaxSize(size_t max_size);

  const http::DestinationStatistics& GetDestinationStatistics() const;

  void SetTestsuiteConfig(const TestsuiteConfig& config);

  void SetConfig(const Config&);

  /// @brief Sets User-Agent headers for all the requests or removes that
  /// header.
  ///
  /// By default User-Agent is set by components::HttpClient to the
  /// userver identity string.
  void ResetUserAgent(std::optional<std::string> user_agent = std::nullopt);

  /// @brief Returns the current proxy that is automatically used for each
  /// request.
  ///
  /// @warning The value may become immediately obsole as the proxy could be
  /// concurrently changed from runtime config.
  std::string GetProxy() const;

 private:
  void ReinitEasy();

  InstanceStatistics GetMultiStatistics(size_t n) const;

  size_t FindMultiIndex(const curl::multi*) const;

  // Functions for EasyWrapper that must be noexcept, as they are called from
  // the EasyWrapper destructor.
  friend class impl::EasyWrapper;
  void IncPending() noexcept { ++pending_tasks_; }
  void DecPending() noexcept { --pending_tasks_; }
  void PushIdleEasy(std::shared_ptr<curl::easy>&& easy) noexcept;

  std::shared_ptr<curl::easy> TryDequeueIdle() noexcept;

  std::atomic<std::size_t> pending_tasks_{0};

  std::shared_ptr<DestinationStatistics> destination_statistics_;
  std::unique_ptr<engine::ev::ThreadPool> thread_pool_;
  std::vector<Statistics> statistics_;
  std::vector<std::unique_ptr<curl::multi>> multis_;

  static constexpr size_t kIdleQueueSize = 616;
  static constexpr size_t kIdleQueueAlignment = 8;
  using IdleQueueTraits = moodycamel::ConcurrentQueueDefaultTraits;
  using IdleQueueValue = std::shared_ptr<curl::easy>;
  using IdleQueue =
      moodycamel::ConcurrentQueue<IdleQueueValue, IdleQueueTraits>;
  utils::FastPimpl<IdleQueue, kIdleQueueSize, kIdleQueueAlignment> idle_queue_;

  engine::TaskProcessor& fs_task_processor_;
  std::optional<std::string> user_agent_;
  rcu::Variable<std::string> proxy_;

  utils::SwappingSmart<const curl::easy> easy_;
  utils::PeriodicTask easy_reinit_task_;

  // Testsuite support
  std::shared_ptr<const TestsuiteConfig> testsuite_config_;

  std::shared_ptr<curl::ConnectRateLimiter> connect_rate_limiter_;
};

}  // namespace clients::http
