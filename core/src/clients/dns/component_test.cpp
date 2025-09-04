#include <userver/utest/utest.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/common_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/component.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/tracing/component.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string kStaticConfig = R"(
components_manager:
  coro_pool:
    initial_size: 50
    max_size: 50000
    stack_usage_monitor_enabled: false
  default_task_processor: main-task-processor
  event_thread_pool:
    threads: 2
  task_processors:
    fs-task-processor:
      thread_name: fs-worker
      worker_threads: 2
    main-task-processor:
      thread_name: main-worker
      worker_threads: 16
  components:
    logging:
      fs-task-processor: fs-task-processor
      loggers:
        default:
          file_path: '@null'
    dns-client:
      fs-task-processor: fs-task-processor
      unknown-field: 123
  )";

using DnsClient = ComponentList;

}  // namespace

TEST_F(DnsClient, InvalidComponentConfig) {
    auto component_list = components::ComponentList()
                              .Append<clients::dns::Component>()
                              .Append<components::Logging>()
                              .Append<components::Tracer>()
                              .Append<components::StatisticsStorage>()
                              .Append<os_signals::ProcessorComponent>();
    UEXPECT_THROW_MSG(
        components::RunOnce(components::InMemoryConfig{kStaticConfig}, component_list),
        std::runtime_error,
        "Cannot start component dns-client: Unknown property 'unknown-field'"
    );
}

USERVER_NAMESPACE_END
