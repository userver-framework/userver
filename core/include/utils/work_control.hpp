#pragma once

#include <atomic>
#include <memory>

#include <engine/sleep.hpp>

namespace utils {

class WorkToken final {
 public:
  bool IsFinished() const { return finished_; }

  void SetFinished() { finished_ = true; }

 private:
  std::atomic<bool> finished_;
};
using WorkTokenPtr = std::shared_ptr<WorkToken>;

class WorkControl final {
 public:
  WorkControl() : token_(std::make_shared<WorkToken>()) {}

  void SetFinished() { token_->SetFinished(); }

  /* Signal WorkToken that the work is done, wait for all workers to finish.
   * It asynchronously polls, so it might finish with a delay.
   */
  template <class Duration>
  void FinishAndWaitForAllWorkers(Duration duration) {
    SetFinished();
    while (AreWorkersAlive()) engine::SleepFor(duration);
  }

  bool AreWorkersAlive() const { return token_.use_count() > 1; }

  WorkTokenPtr GetToken() { return token_; }

 private:
  const WorkTokenPtr token_;
};

}  // namespace utils
