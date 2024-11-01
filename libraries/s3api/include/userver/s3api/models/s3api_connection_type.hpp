#pragma once

/// @file userver/s3api/models/s3api_connection_type.hpp
/// @brief S3 connection type. Currently used to distinguish between http and
///        https. It will be removed in future versions

#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

enum class S3ConnectionType {
    kHttp,
    kHttps,
};

S3ConnectionType Parse(const formats::json::Value& elem, formats::parse::To<S3ConnectionType>);

std::string ToString(S3ConnectionType connection_type);
std::string_view ToStringView(S3ConnectionType connection_type);

}  // namespace s3api

USERVER_NAMESPACE_END
