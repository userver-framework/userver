#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

struct ExecutionResult final {
  std::size_t rows_affected{};

  std::size_t last_insert_id{};
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
