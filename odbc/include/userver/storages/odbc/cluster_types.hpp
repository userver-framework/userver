#pragma once

#include <string>

#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}  // namespace logging

namespace storages::odbc {

enum class ClusterHostType : uint8_t {
    /// @name Cluster roles
    /// @{
    /// No host role detected yet.
    kNone = 0x00,
    /// Connect to cluster's master. Only this connection may be
    /// used for read-write transactions.
    kMaster = 0x01,
    /// Connect to one of cluster's slaves. Can be used
    /// only for read only transactions.
    kSlave = 0x02,
    /// @}
};

using ClusterHostTypeFlags = USERVER_NAMESPACE::utils::Flags<ClusterHostType>;

constexpr ClusterHostTypeFlags kClusterHostRolesMask{ClusterHostType::kMaster, ClusterHostType::kSlave};

std::string ToString(ClusterHostType);
std::string ToString(ClusterHostTypeFlags);
logging::LogHelper& operator<<(logging::LogHelper&, ClusterHostType);
logging::LogHelper& operator<<(logging::LogHelper&, ClusterHostTypeFlags);

}  // namespace storages::odbc

USERVER_NAMESPACE_END
