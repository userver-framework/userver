#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

enum class IteratorDirection {
  kForward = 1,
  kReverse = -1,
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
