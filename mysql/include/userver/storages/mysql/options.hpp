#pragma once

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

using TimeoutDuration = std::chrono::milliseconds;

struct CommandControl final {
  TimeoutDuration execute{};
};

using OptionalCommandControl = std::optional<CommandControl>;

}  // namespace storages::mysql

USERVER_NAMESPACE_END
