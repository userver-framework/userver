#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

enum class HostConnectionState {
    kAlive,
    kChecking,
    kDead,
};

HostConnectionState CheckTcpConnectionState(const char* host_and_port);

void ReportTcpConnectSuccess(const char* host_and_port);
void ReportTcpConnectError(const char* host_and_port);

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
