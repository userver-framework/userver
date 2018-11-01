#include <storages/postgres/cluster_types.hpp>

#include <iostream>

#include <storages/postgres/dsn.hpp>

namespace storages {
namespace postgres {

namespace {

const char* ToStringRaw(const ClusterHostType& ht) {
#define CHT_TO_STR(kType)      \
  case ClusterHostType::kType: \
    return "ClusterHostType::" #kType

  switch (ht) {
    CHT_TO_STR(kMaster);
    CHT_TO_STR(kSyncSlave);
    CHT_TO_STR(kSlave);
    CHT_TO_STR(kAny);
  }

#undef CHT_TO_STR
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const ClusterHostType& ht) {
  return os << ToStringRaw(ht);
}

std::string ToString(const ClusterHostType& ht) { return ToStringRaw(ht); }

ClusterDescription::ClusterDescription(const std::string& multi_host_dsn) {
  auto dsn_list = storages::postgres::SplitByHost(multi_host_dsn);
  if (!dsn_list.empty()) {
    master_dsn_ = dsn_list[0];
    dsn_list.erase(dsn_list.begin());
  }
  if (!dsn_list.empty()) {
    sync_slave_dsn_ = dsn_list[0];
    dsn_list.erase(dsn_list.begin());
  }
  slave_dsns_ = std::move(dsn_list);
}

ClusterDescription::ClusterDescription(const std::string& master_dsn,
                                       const std::string& sync_slave_dsn,
                                       DSNList slave_dsns)
    : master_dsn_(master_dsn),
      sync_slave_dsn_(sync_slave_dsn),
      slave_dsns_(std::move(slave_dsns)) {}

}  // namespace postgres
}  // namespace storages
