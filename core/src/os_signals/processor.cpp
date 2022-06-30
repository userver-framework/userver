#include <userver/os_signals/processor.hpp>

#include <userver/logging/log.hpp>
#include <utils/internal_tag.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

namespace {
constexpr std::string_view kOsSignalProcessorChannelName =
    "os-signal-processor-channel";
}

Processor::Processor(engine::TaskProcessor& task_processor)
    : channel_(kOsSignalProcessorChannelName.data()),
      task_processor_(task_processor) {}

void Processor::Notify(int signum, utils::InternalTag) {
  LOG_DEBUG() << "Send event: " << utils::strsignal(signum);
  engine::AsyncNoSpan(task_processor_, [this, signum] {
    channel_.SendEvent(signum);
  }).BlockingWait();
}

}  // namespace os_signals

USERVER_NAMESPACE_END
