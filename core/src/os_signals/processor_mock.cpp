#include <userver/os_signals/processor_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

ProcessorMock::ProcessorMock(engine::TaskProcessor& task_processor) : manager_(task_processor) {}

Processor& ProcessorMock::Get() { return manager_; }

void ProcessorMock::Notify(int signum) { manager_.Notify(signum, utils::impl::InternalTag{}); }

}  // namespace os_signals

USERVER_NAMESPACE_END
