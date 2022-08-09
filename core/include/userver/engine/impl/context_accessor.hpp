#pragma once

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;
class GenericWaitScope;

class ContextAccessor {
 public:
  ContextAccessor();

  virtual bool IsReady() const noexcept = 0;

  virtual GenericWaitScope MakeWaitScope(impl::TaskContext& context) = 0;

  virtual void RethrowErrorResult() const = 0;

 protected:
  ~ContextAccessor() = default;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
