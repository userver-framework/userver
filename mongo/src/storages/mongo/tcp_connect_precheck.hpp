#pragma once

namespace storages::mongo::impl {

void SetTcpConnectPrecheckEnabled(bool enabled);

bool IsTcpConnectAllowed(const char* host_and_port);

void ReportTcpConnectSuccess(const char* host_and_port);
void ReportTcpConnectError(const char* host_and_port);

}  // namespace storages::mongo::impl
