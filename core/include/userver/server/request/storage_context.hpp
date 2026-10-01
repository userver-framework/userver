#pragma once

/// @file userver/server/request/storage_context.hpp
/// @brief @copybrief server::request::StorageContext

#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

/// @brief AnyStorage tag for HTTP request context
/// @see RequestContext
struct StorageContext {};

}  // namespace server::request

USERVER_NAMESPACE_END
