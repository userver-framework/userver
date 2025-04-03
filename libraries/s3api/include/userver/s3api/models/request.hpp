#pragma once

/// @file userver/s3api/models/request.hpp
/// @brief Request class. Although it is rarely used in S3 client, it is
///        required to implement authenticator

#include <userver/clients/http/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

struct Request {
    clients::http::Headers headers;
    std::string body;
    std::string bucket;
    std::string req;
    clients::http::HttpMethod method;
};

}  // namespace s3api

USERVER_NAMESPACE_END
