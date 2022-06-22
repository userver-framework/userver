#pragma once

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class ContextAccessor {
 public:
  ContextAccessor();

  virtual bool IsReady() const noexcept = 0;

  virtual void AppendWaiter(impl::TaskContext& context) noexcept = 0;

  virtual void RemoveWaiter(impl::TaskContext& context) noexcept = 0;

  virtual void RethrowErrorResult() const = 0;

 protected:
  ~ContextAccessor() = default;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
