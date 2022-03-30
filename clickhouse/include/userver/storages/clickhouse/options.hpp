#pragma once

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

struct CommandControl final {
  std::chrono::milliseconds execute;

  explicit constexpr CommandControl(std::chrono::milliseconds execute)
      : execute{execute} {}
};

using OptionalCommandControl = std::optional<CommandControl>;

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
