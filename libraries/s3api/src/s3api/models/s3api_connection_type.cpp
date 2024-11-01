#include <userver/s3api/models/s3api_connection_type.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

S3ConnectionType Parse(const formats::json::Value& elem, formats::parse::To<S3ConnectionType>) {
    const auto as_string = elem.As<std::string>();
    if (as_string == "http") {
        return S3ConnectionType::kHttp;
    }
    if (as_string == "https") {
        return S3ConnectionType::kHttps;
    }
    UINVARIANT(false, "invalid value of connection_type");
}

std::string_view ToStringView(S3ConnectionType connection_type) {
    switch (connection_type) {
        case S3ConnectionType::kHttp:
            return "http";
        case S3ConnectionType::kHttps:
            return "https";
    }
    return "unknown";
}

std::string ToString(S3ConnectionType connection_type) { return std::string{ToStringView(connection_type)}; }

}  // namespace s3api

USERVER_NAMESPACE_END
