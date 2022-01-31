#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

/// The engine-facing side of an asynchronous task payload. The engine will
/// call `Perform` or `Reset` at most once.
class WrappedCallBase {
 public:
  WrappedCallBase(WrappedCallBase&&) = delete;

  /// Invoke the wrapped function call, then release the resources held
  virtual void Perform() = 0;

  /// Release the resources held
  virtual void Reset() noexcept = 0;

 protected:
  WrappedCallBase() noexcept;

  // disallow destruction via pointer to base
  ~WrappedCallBase();
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
