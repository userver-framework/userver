#pragma once

/// @file userver/server/request/json_data.hpp
/// @brief Functions that handle JSON requests and responses stored in context.

#include <userver/formats/json/value.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

/// @returns A pointer to json request if it was parsed successfully and stored in the context or
/// nullptr otherwise.
const formats::json::Value* GetRequestJson(const RequestContext& context);

/// @brief Saves a parsed json request to the context.
/// @returns A reference to the saved json.
formats::json::Value& SetRequestJson(RequestContext& context, formats::json::Value request_json);

/// @returns A pointer to json response if it was produced successfully and stored in the context
/// or nullptr otherwise.
const formats::json::Value* GetResponseJson(const RequestContext& context);

/// @brief Saves a json response to the context.
/// @returns A reference to the saved json.
formats::json::Value& SetResponseJson(RequestContext& context, formats::json::Value request_json);

}  // namespace server::request

USERVER_NAMESPACE_END
