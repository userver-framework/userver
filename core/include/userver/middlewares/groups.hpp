#pragma once

/// @file userver/middlewares/groups.hpp
/// @brief
/// There are groups of middlewares to build a pipeline.
/// @see @ref scripts/docs/en/userver/grpc_server_middlewares.md

#include <userver/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

/// Middlewares groups for middlewares pipeline.
/// @see @ref scripts/docs/en/userver/grpc_server_middlewares.md
namespace middlewares::groups {

/// @brief The first group in the pipeline.
struct PreCore final {
    static constexpr std::string_view kName = "pre-core";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder();
};

/// @brief The Group to work wih logging. Is located after PreCore.
///
/// @details There are:
///
/// gRPC-server:
/// ugrpc::server::middlewares::log::Component.
/// gRPC-client:
/// ugrpc::client::middlewares::log::Component.
struct Logging final {
    static constexpr std::string_view kName = "logging";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<PreCore>();
};

/// @brief The Group for authentication middlewares. Is located after `Logging`.
struct Auth final {
    static constexpr std::string_view kName = "auth";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<Logging>();
};

/// @brief The core group of middlewares. Is located after `Auth`.
///
/// @details There are:
///
/// gRPC-server:
/// * ugrpc::server::middlewares::congestion_control::Component
/// * ugrpc::server::middlewares::deadline_propagation::Component
/// gRPC-client:
/// * ugrpc::client::middlewares::deadline_propagation::Component
struct Core final {
    static constexpr std::string_view kName = "core";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<Auth>();
};

/// @brief The group is located after `Core`.
///
/// @details There are:
///
/// gRPC-client:
/// * ugrpc::client::middlewares::testsuite::Component
struct PostCore final {
    static constexpr std::string_view kName = "post-core";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<Core>();
};

/// @brief The group for user middlewares - the last group in pipeline. It group used by default.
///
/// @details There are:
///
/// gRPC-server:
/// * ugrpc::server::middlewares::baggage::Component
/// * ugrpc::server::middlewares::headers_propagator::Component
/// gRPC-client:
/// * ugrpc::client::middlewares::baggage::Component
struct User final {
    static constexpr std::string_view kName = "user";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<PostCore>();
};

}  // namespace middlewares::groups

USERVER_NAMESPACE_END
