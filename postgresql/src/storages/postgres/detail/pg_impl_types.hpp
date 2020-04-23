#pragma once

namespace storages::postgres::detail {

enum class DefaultCommandControlSource {
  kGlobalConfig,
  kUser,
};

}  // namespace storages::postgres::detail
