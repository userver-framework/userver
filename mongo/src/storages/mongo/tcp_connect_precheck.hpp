#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

bool IsTcpConnectAllowed(const char* host_and_port);

void ReportTcpConnectSuccess(const char* host_and_port);
void ReportTcpConnectError(const char* host_and_port);

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
