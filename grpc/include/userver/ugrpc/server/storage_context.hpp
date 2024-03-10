#pragma once

/// @file userver/ugrpc/server/storage_context.hpp
/// @brief StorageContext

#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief AnyStorage tag for gRPC call context
/// @see CallAnyBase::GetStorageContext
struct StorageContext {};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
