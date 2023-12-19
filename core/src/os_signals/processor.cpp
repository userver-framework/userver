#include <userver/os_signals/processor.hpp>

#include <csignal>  // for SIGUSR1, SIGUSR2

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <utils/internal_tag.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

namespace {
constexpr std::string_view kOsSignalProcessorChannelName =
    "os-signal-processor-channel";

static_assert(SIGUSR1 == kSigUsr1);
static_assert(SIGUSR2 == kSigUsr2);
}  // namespace

Processor::Processor(engine::TaskProcessor& task_processor)
    : channel_(kOsSignalProcessorChannelName.data()),
      task_processor_(task_processor) {}

void Processor::Notify(int signum, utils::InternalTag) {
  UINVARIANT(signum == SIGUSR1 || signum == SIGUSR2,
             "Processor::Notify() should be used only with SIGUSR1 and SIGUSR2 "
             "signum values");
  LOG_DEBUG() << "Send event: " << utils::strsignal(signum);
  if (engine::current_task::IsTaskProcessorThread()) {
    channel_.SendEvent(signum);
  } else {
    engine::AsyncNoSpan(task_processor_, [this, signum] {
      channel_.SendEvent(signum);
    }).BlockingWait();
  }
}

}  // namespace os_signals

USERVER_NAMESPACE_END
