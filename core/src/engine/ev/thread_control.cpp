#include <engine/ev/thread_control.hpp>

#include <iostream>

#include <engine/single_consumer_event.cpp>
#include <engine/task/cancel.hpp>
#include <utils/userver_experiment.hpp>

namespace engine::ev {

void ThreadControl::RunInEvLoopSync(std::function<void()>&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  auto event = std::make_shared<engine::SingleConsumerEvent>();
  RunInEvLoopAsync([event, func = std::move(func)] {
    func();
    event->Send();
  });

  engine::TaskCancellationBlocker cancellation_blocker;
  if (utils::IsUserverExperimentEnabled(
          utils::UserverExperiment::kTaxicommon1479)) {
    static const auto kSyncExecTimeout = std::chrono::minutes{2};
    if (!event->WaitForEventFor(kSyncExecTimeout)) {
      std::cerr << "Aborting due to sync exec timeout in ev thread\n";
      abort();
    }
  } else {
    [[maybe_unused]] auto result = event->WaitForEvent();
    UASSERT(result);
  }
}

}  // namespace engine::ev
