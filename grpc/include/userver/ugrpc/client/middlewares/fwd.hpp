#pragma once

/// @file userver/ugrpc/client/middlewares/fwd.hpp
/// @brief Forward declarations for grpc server middlewares

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

class MiddlewareBase;
class MiddlewareFactoryBase;

/// @brief A chain of middlewares
using Middlewares = std::vector<std::shared_ptr<const MiddlewareBase>>;

using MiddlewareFactories =
    std::vector<std::shared_ptr<const MiddlewareFactoryBase>>;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
