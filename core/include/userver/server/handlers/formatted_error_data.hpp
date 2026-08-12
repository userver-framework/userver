#pragma once

/// @file userver/server/handlers/formatted_error_data.hpp
/// @brief @copybrief server::handlers::FormattedErrorData

#include <optional>
#include <string>

#include <userver/http/content_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @brief External error response body and content type
struct FormattedErrorData {
    std::string external_body;
    std::optional<USERVER_NAMESPACE::http::ContentType> content_type{};
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
