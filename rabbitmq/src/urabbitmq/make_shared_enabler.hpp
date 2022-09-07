#pragma once

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

template <class Base>
class MakeSharedEnabler final : public Base {
 public:
  template <typename... Args>
  MakeSharedEnabler(Args&&... args) : Base{std::forward<Args>(args)...} {}
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
