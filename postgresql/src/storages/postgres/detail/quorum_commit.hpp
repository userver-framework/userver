#pragma once

#include <unordered_map>
#include <vector>

#include <error_injection/settings.hpp>
#include <rcu/rcu.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/options.hpp>
#include <testsuite/postgres_control.hpp>
#include <utils/fast_pimpl.hpp>

#include <engine/task/task_processor.hpp>

namespace storages::postgres::detail {

class QuorumCommitTopology {
 public:
  using DsnIndex = size_t;
  using DsnIndices = std::vector<DsnIndex>;
  using DsnIndicesByType =
      std::unordered_map<ClusterHostType, DsnIndices, ClusterHostTypeHash>;

  QuorumCommitTopology(engine::TaskProcessor& bg_task_processor, DsnList dsns,
                       const ConnectionSettings& conn_settings,
                       const CommandControl& default_cmd_ctl,
                       const testsuite::PostgresControl& testsuite_pg_ctl,
                       error_injection::Settings ei_settings);

  ~QuorumCommitTopology();

  const DsnList& GetDsnList() const;
  rcu::ReadablePtr<DsnIndicesByType> GetDsnIndicesByType() const;
  rcu::ReadablePtr<DsnIndices> GetDsnIndicesByRoundtripTime() const;

 private:
  class Impl;
  // MAC_COMPAT
#ifdef _LIBCPP_VERSION
  static constexpr std::size_t kImplSize = 976;
  static constexpr std::size_t kImplAlign = 16;
#else
  static constexpr std::size_t kImplSize = 864;
  static constexpr std::size_t kImplAlign = 8;
#endif
  ::utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

}  // namespace storages::postgres::detail
