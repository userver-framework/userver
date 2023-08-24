#pragma once

/// @file userver/ugrpc/server/middlewares/fwd.hpp
/// @brief Forward declarations for grpc server middlewares

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class MiddlewareBase;

/// @brief A chain of middlewares
using Middlewares = std::vector<std::shared_ptr<MiddlewareBase>>;

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
