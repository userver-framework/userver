#pragma once

#include <atomic>
#include <memory>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

// The base class for data for an asynchronous ev call that knows how to
// "delete" itself after the call.
class AsyncPayloadBase {
 public:
  struct Deleter final {
    void operator()(AsyncPayloadBase* payload) noexcept {
      payload->release_func_(*payload);
    }
  };

  AsyncPayloadBase(AsyncPayloadBase&&) = delete;
  AsyncPayloadBase& operator=(AsyncPayloadBase&&) = delete;

 protected:
  using ReleaseFunc = void (*)(AsyncPayloadBase&) noexcept;

  explicit AsyncPayloadBase(ReleaseFunc release_func) noexcept
      : release_func_(release_func) {}

  ~AsyncPayloadBase() = default;

  static void Noop(AsyncPayloadBase&) noexcept {}

 private:
  const ReleaseFunc release_func_;
};

using AsyncPayloadPtr =
    std::unique_ptr<AsyncPayloadBase, AsyncPayloadBase::Deleter>;

using OnAsyncPayload = void(AsyncPayloadPtr&& ptr);

}  // namespace engine::ev

USERVER_NAMESPACE_END
