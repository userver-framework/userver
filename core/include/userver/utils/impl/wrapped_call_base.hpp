#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

/// The engine-facing side of an asynchronous task payload. The engine will
/// call `Perform` or `Reset` at most once.
class WrappedCallBase {
 public:
  WrappedCallBase(WrappedCallBase&&) = delete;
  virtual ~WrappedCallBase();

  /// Invoke the wrapped function call, then release the resources held
  virtual void Perform() = 0;

 protected:
  WrappedCallBase() noexcept;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
