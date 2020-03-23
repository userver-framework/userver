#pragma once

#include <error_injection/settings.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/options.hpp>
#include <testsuite/postgres_control.hpp>

#include <engine/task/task_processor.hpp>

#include <utils/fast_pimpl.hpp>

namespace storages::postgres::detail {

class QuorumCommitCluster {
 public:
  using HostsByType =
      std::unordered_map<ClusterHostType, DSNList, ClusterHostTypeHash>;

  QuorumCommitCluster(engine::TaskProcessor& bg_task_processor,
                      const DSNList& dsns, ConnectionSettings conn_settings,
                      CommandControl default_cmd_ctl,
                      const testsuite::PostgresControl& testsuite_pg_ctl,
                      const error_injection::Settings& ei_settings);

  ~QuorumCommitCluster();

  const DSNList& GetDsnList() const;
  HostsByType GetHostsByType() const;
  DSNList GetHostsByRoundtripTime() const;

 private:
  struct Impl;
  // MAC_COMPAT
#ifdef _LIBCPP_VERSION
  static constexpr std::size_t kImplSize = 944;
  static constexpr std::size_t kImplAlign = 16;
#else
  static constexpr std::size_t kImplSize = 832;
  static constexpr std::size_t kImplAlign = 8;
#endif
  ::utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

using QuorumCommitTopologyPtr = std::unique_ptr<QuorumCommitCluster>;

}  // namespace storages::postgres::detail
