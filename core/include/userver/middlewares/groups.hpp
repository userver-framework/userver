#pragma once

/// @file userver/middlewares/groups.hpp
/// @brief
/// There are groups of middlewares to build a pipeline.
/// @see @ref scripts/docs/en/userver/grpc/middlewares_order.md

#include <userver/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

/// Middlewares groups for a middlewares pipeline.
/// @see @ref scripts/docs/en/userver/grpc/middlewares_order.md
namespace middlewares::groups {

/// @ingroup userver_middlewares_groups
///
/// @brief The first group in the pipeline.
struct PreCore final {
    static constexpr std::string_view kName = "pre-core";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder();
};

/// @ingroup userver_middlewares_groups
///
/// @brief The Group to work with logging. Is located after PreCore.
///
/// @details There are:
///
/// gRPC-server:
/// * @ref ugrpc::server::middlewares::log::Component.
/// gRPC-client:
/// * @ref ugrpc::client::middlewares::log::Component.
struct Logging final {
    static constexpr std::string_view kName = "logging";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<PreCore>();
};

/// @ingroup userver_middlewares_groups
///
/// @brief The Group for authentication middlewares. Is located after `Logging`.
struct Auth final {
    static constexpr std::string_view kName = "auth";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<Logging>();
};

/// @ingroup userver_middlewares_groups
///
/// @brief The core group of middlewares. Is located after `Auth`.
///
/// @details There are:
///
/// gRPC-server:
/// * @ref ugrpc::server::middlewares::congestion_control::Component
/// * @ref ugrpc::server::middlewares::deadline_propagation::Component
/// gRPC-client:
/// * @ref ugrpc::client::middlewares::deadline_propagation::Component
struct Core final {
    static constexpr std::string_view kName = "core";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<Auth>();
};

/// @ingroup userver_middlewares_groups
///
/// @brief The group is located after `Core`.
///
/// @details There are:
///
/// gRPC-client:
/// * @ref ugrpc::client::middlewares::testsuite::Component
struct PostCore final {
    static constexpr std::string_view kName = "post-core";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<Core>();
};

/// @ingroup userver_middlewares_groups
///
/// @brief The group for user middlewares - the last group in pipeline. It group used by default.
///
/// @details There are:
///
/// gRPC-server:
/// * @ref ugrpc::server::middlewares::baggage::Component
/// * @ref ugrpc::server::middlewares::headers_propagator::Component
/// gRPC-client:
/// * @ref ugrpc::client::middlewares::baggage::Component
/// * @ref ugrpc::client::middlewares::headers_propagator::Component
/// * @ref ugrpc::client::middlewares::origin::Component
struct User final {
    static constexpr std::string_view kName = "user";
    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder().After<PostCore>();
};

}  // namespace middlewares::groups

USERVER_NAMESPACE_END
