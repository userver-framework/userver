#pragma once

#include <cstdint>
#include <string>

#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;

struct PoolConnectionStatistics final {
  Counter overload{};
  Counter closed{};
  Counter created{};
  Counter acquired{};
  Counter released{};
};

struct PoolStatistics final {
  std::string type;

  PoolConnectionStatistics connections{};
};

void DumpMetric(utils::statistics::Writer& writer, const PoolStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats);

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
