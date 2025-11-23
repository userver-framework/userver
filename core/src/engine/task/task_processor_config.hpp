#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include <userver/engine/run_standalone.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <dynamic_config/variables/USERVER_TASK_PROCESSOR_PROFILER_DEBUG.hpp>
#include <dynamic_config/variables/USERVER_TASK_PROCESSOR_QOS.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

enum class OsScheduling {
    kNormal,
    kLowPriority,
    kIdle,
};

OsScheduling Parse(const yaml_config::YamlConfig& value, formats::parse::To<OsScheduling>);

struct TaskProcessorConfig {
    std::string name;

    bool should_guess_cpu_limit{false};
    std::size_t worker_threads{6};
    std::string thread_name;
    OsScheduling os_scheduling{OsScheduling::kNormal};
    int spinning_iterations{1000};
    TaskQueueType task_processor_queue{TaskQueueType::kGlobalTaskQueue};

    std::size_t task_trace_every{1000};
    std::size_t task_trace_max_csw{0};
    std::string task_trace_logger_name;

    bool trace_coroutines{false};

    void SetName(const std::string& new_name);
};

TaskProcessorConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<TaskProcessorConfig>);

using TaskProcessorSettings = ::dynamic_config::userver_task_processor_qos::DefaultTaskProcessor;

using TaskProcessorProfilerSettings = ::dynamic_config::userver_task_processor_profiler_debug::TaskProcessorSettings;

using TaskProcessorSettingsOverloadAction = ::dynamic_config::userver_task_processor_qos::WaitQueueOverload::Action;

}  // namespace engine

USERVER_NAMESPACE_END
