#pragma once

/// @file userver/os_signals/processor_mock.hpp
/// @brief @copybrief os_signals::ProcessorMock

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/os_signals/processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

/// @brief Provides Processor for use in tests
class ProcessorMock final {
 public:
  explicit ProcessorMock(engine::TaskProcessor& task_processor);

  Processor& Get();

  void Notify(int signum);

 private:
  Processor manager_;
};

}  // namespace os_signals

USERVER_NAMESPACE_END
