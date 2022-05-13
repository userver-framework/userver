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

// The base class for ref-counted data for an asynchronous ev call.
template <typename Derived>
class RefCountedAsyncPayloadBase : public AsyncPayloadBase {
 public:
  RefCountedAsyncPayloadBase() noexcept : AsyncPayloadBase(&Release) {}

  friend void intrusive_ptr_add_ref(RefCountedAsyncPayloadBase* ptr) noexcept {
    ++ptr->ref_counter_;
  }

  friend void intrusive_ptr_release(RefCountedAsyncPayloadBase* ptr) noexcept {
    if (--ptr->ref_counter_ == 0) delete static_cast<Derived*>(ptr);
  }

  AsyncPayloadPtr SelfAsPayload() noexcept {
    intrusive_ptr_add_ref(this);
    return AsyncPayloadPtr(this);
  }

 private:
  static void Release(AsyncPayloadBase& data) noexcept {
    intrusive_ptr_release(&static_cast<RefCountedAsyncPayloadBase&>(data));
  }

  std::atomic<std::uint64_t> ref_counter_{0};
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
