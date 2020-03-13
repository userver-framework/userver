#include <engine/ev/thread_control.hpp>

#include <future>
#include <iostream>

#include <engine/single_consumer_event.hpp>
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

void ThreadControl::RunInEvLoopBlocking(std::function<void()>&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  auto func_promise = std::make_shared<std::promise<void>>();
  auto func_future = func_promise->get_future();
  RunInEvLoopAsync(
      [promise = std::move(func_promise), func = std::move(func)]() {
        func();
        promise->set_value();
      });
  func_future.get();
}

}  // namespace engine::ev
