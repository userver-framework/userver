#pragma once

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

template <typename Base>
class MakeSharedEnabler final : public Base {
 public:
  template <typename... Args>
  MakeSharedEnabler(Args&&... args) : Base{std::forward<Args>(args)...} {}
};

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
