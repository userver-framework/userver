#pragma once

/// @file userver/clients/http/client.hpp
/// @brief @copybrief clients::http::Client

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use clients::Http from clients/http.hpp instead
#endif

#include <memory>

#include <userver/moodycamel/concurrentqueue_fwd.h>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/clients/http/plugin.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/swappingsmart.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class TracingManagerBase;
}  // namespace tracing

namespace server::http {
class HeadersPropagator;
}  // namespace server::http

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

struct Config;
struct TestsuiteConfig;
struct EnforceTaskDeadlineConfig;
class Statistics;
struct PoolStatistics;
struct InstanceStatistics;
class DestinationStatistics;

struct ClientSettings final {
  std::string thread_name_prefix;
  size_t io_threads = 8;
  bool defer_events = false;
  const tracing::TracingManagerBase* tracing_manager_{nullptr};
  const server::http::HeadersPropagator* headers_propagator_{nullptr};
};

ClientSettings Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ClientSettings>);

/// @ingroup userver_clients
///
/// @brief HTTP client that returns a HTTP request builder from
/// CreateRequest().
///
/// Usually retrieved from components::HttpClient component.
///
/// ## Example usage:
///
/// @snippet clients/http/client_test.cpp  Sample HTTP Client usage
class Client final {
 public:
  Client(ClientSettings settings, engine::TaskProcessor& fs_task_processor,
         impl::PluginPipeline&& plugin_pipeline);
  ~Client();

  /// @brief Returns a HTTP request builder type with preset values of
  /// User-Agent, Proxy and some of the Testsuite suff (if any).
  ///
  /// @note This method is thread-safe despite being non-const.
  Request CreateRequest();

  /// Providing CreateNonSignedRequest() function for the clients::Http alias.
  ///
  /// @note This method is thread-safe despite being non-const.
  Request CreateNotSignedRequest() { return CreateRequest(); }

  /// @cond
  // For internal use only.
  void SetMultiplexingEnabled(bool enabled);

  // For internal use only.
  void SetMaxHostConnections(size_t max_host_connections);

  // For internal use only.
  PoolStatistics GetPoolStatistics() const;

  // Set max number of automatically created destination metrics.
  // For internal use only.
  void SetDestinationMetricsAutoMaxSize(size_t max_size);

  // For internal use only.
  const http::DestinationStatistics& GetDestinationStatistics() const;

  // For internal use only.
  void SetTestsuiteConfig(const TestsuiteConfig& config);

  // For internal use only.
  void SetAllowedUrlsExtra(std::vector<std::string>&& urls);

  // For internal use only.
  void SetConfig(const Config&);
  /// @endcond

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

  /// @brief Sets the DNS resolver to use.
  ///
  /// If given nullptr, the default resolver will be used
  /// (most likely getaddrinfo).
  void SetDnsResolver(clients::dns::Resolver* resolver);

  void SetTracingManager(const tracing::TracingManagerBase&);

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
  rcu::Variable<EnforceTaskDeadlineConfig> enforce_task_deadline_;

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
  rcu::Variable<std::vector<std::string>> allowed_urls_extra_;

  std::shared_ptr<curl::ConnectRateLimiter> connect_rate_limiter_;

  clients::dns::Resolver* resolver_{nullptr};
  utils::NotNull<const tracing::TracingManagerBase*> tracing_manager_;
  const server::http::HeadersPropagator* headers_propagator_{nullptr};
  impl::PluginPipeline plugin_pipeline_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
