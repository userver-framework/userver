#pragma once

/// @file userver/ugrpc/server/middlewares/fwd.hpp
/// @brief Forward declarations for grpc server middlewares

#include <memory>
#include <vector>

#include <boost/container/small_vector.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class MiddlewareBase;
// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class MiddlewareCallContext;

/// @brief A chain of middlewares
using Middlewares =
    boost::container::small_vector<std::shared_ptr<MiddlewareBase>, 5>;

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
