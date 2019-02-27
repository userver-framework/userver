#pragma once

#include <chrono>
#include <memory>

#include "thread_control.hpp"

#include <utils/assert.hpp>

namespace engine {
namespace ev {

template <typename Impl>
class ImplInEvLoop : public ThreadControl {
 public:
  template <typename... Args>
  explicit ImplInEvLoop(const ThreadControl& thread_control, Args&&... args)
      : ThreadControl(thread_control) {
    CreateInEvLoop(thread_control, std::forward<Args>(args)...);
    UASSERT(impl_);
  }

  virtual ~ImplInEvLoop() {
    DestroyInEvLoop();
    UASSERT(!impl_);
  }

 protected:
  std::unique_ptr<Impl> impl_;

 private:
  template <typename... Args>
  void CreateInEvLoop(Args&&... args) {
    RunInEvLoopSync([this, &args...]() {
      impl_ = std::make_unique<Impl>(std::forward<Args>(args)...);
    });
  }

  void DestroyInEvLoop() {
    RunInEvLoopSync([this]() { impl_.reset(); });
  }
};

}  // namespace ev
}  // namespace engine
