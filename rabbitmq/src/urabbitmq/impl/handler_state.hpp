#pragma once

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class HandlerState final {
 public:
  void SetBroken();
  void SetBlocked();
  void SetUnblocked();

  bool IsBroken() const;
  bool IsBlocked() const;

 private:
  std::atomic<bool> broken_{false};
  std::atomic<bool> blocked_{false};
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
