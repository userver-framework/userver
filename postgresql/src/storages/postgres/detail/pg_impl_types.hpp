#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

enum class DefaultCommandControlSource {
  kGlobalConfig,
  kUser,
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
