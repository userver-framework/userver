#pragma once

#include <string>
#include <string_view>

#include <ydb-cpp-sdk/client/driver/fwd.h>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

/// Normalizes userver version to X.Y.Z format required by NYdb::TDriverConfig::AppendBuildInfo.
std::string FormatUserverVersionForBuildInfo(std::string_view version);

/// Appends "ydb-userver/<version>" segment to the driver config build info header.
void AppendUserverYdbBuildInfo(NYdb::TDriverConfig& config);

}  // namespace ydb::impl

USERVER_NAMESPACE_END
