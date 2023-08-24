#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {
class BlockWrapper;

struct BlockWrapperDeleter final {
  void operator()(BlockWrapper* ptr) const noexcept;
};

using BlockWrapperPtr = std::unique_ptr<BlockWrapper, BlockWrapperDeleter>;

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
