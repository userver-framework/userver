#include <userver/storages/postgres/cluster_types.hpp>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

const char* ToStringRaw(ClusterHostType ht) {
  switch (ht) {
    case ClusterHostType::kNone:
      return "default";

    case ClusterHostType::kMaster:
      return "master";
    case ClusterHostType::kSyncSlave:
      return "sync slave";
    case ClusterHostType::kSlave:
      return "slave";
    case ClusterHostType::kSlaveOrMaster:
      return "slave|master";

    case ClusterHostType::kRoundRobin:
      return "round-robin";
    case ClusterHostType::kNearest:
      return "nearest";
  }
  const auto msg = fmt::format("invalid host type {} in ToStringRaw",
                               USERVER_NAMESPACE::utils::UnderlyingValue(ht));
  UASSERT_MSG(false, msg);
  throw LogicError(msg);
}

}  // namespace

std::string ToString(ClusterHostType ht) { return ToStringRaw(ht); }

std::string ToString(ClusterHostTypeFlags flags) {
  std::string result;

  for (const auto role : {ClusterHostType::kMaster, ClusterHostType::kSyncSlave,
                          ClusterHostType::kSlave, ClusterHostType::kRoundRobin,
                          ClusterHostType::kNearest}) {
    if (flags & role) {
      if (!result.empty()) result += '|';
      result += ToStringRaw(role);
    }
  }
  if (result.empty()) result = ToStringRaw(ClusterHostType::kNone);
  return result;
}

logging::LogHelper& operator<<(logging::LogHelper& os, ClusterHostType ht) {
  return os << ToStringRaw(ht);
}

logging::LogHelper& operator<<(logging::LogHelper& os,
                               ClusterHostTypeFlags flags) {
  return os << ToString(flags);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
