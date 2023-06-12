#pragma once

#include <cstdint>

#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;

struct PoolConnectionStatistics final {
  Counter overload{};
  Counter closed{};
  Counter created{};
  Counter acquired{};
  Counter released{};
};

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats);

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
