#pragma once

/// @file userver/ydb/coordination.hpp
/// @brief YDB Coordination client

#include <memory>
#include <string_view>

#include <ydb-cpp-sdk/client/coordination/coordination.h>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
}  // namespace impl

/// @brief Coordination Session
///
/// @see https://ydb.tech/docs/ru/reference/ydb-sdk/coordination#session
class CoordinationSession final {
public:
    /// @cond
    // For internal use only.
    explicit CoordinationSession(NYdb::NCoordination::TSession&& session);
    /// @endcond

    /// Get session id
    std::uint64_t GetSessionId();

    /// Get session state
    NYdb::NCoordination::ESessionState GetSessionState();

    /// Get connection state
    NYdb::NCoordination::EConnectionState GetConnectionState();

    /// Close session
    void Close();

    /// Ping
    void Ping();

    /// Reconnect session
    void Reconnect();

    /// Acquire semaphore
    /// @warning Use `TAcquireSemaphoreSettings::OnAccepted` callback with care,
    /// it will be executed on a non-coroutine thread
    bool AcquireSemaphore(std::string_view name, const NYdb::NCoordination::TAcquireSemaphoreSettings& settings);

    /// Release semaphore
    bool ReleaseSemaphore(std::string_view name);

    /// Describe semaphore
    /// @warning Use `TDescribeSemaphoreSettings::OnChanged` callback with care,
    /// it will be executed on a non-coroutine thread
    NYdb::NCoordination::TSemaphoreDescription
    DescribeSemaphore(std::string_view name, const NYdb::NCoordination::TDescribeSemaphoreSettings& settings);

    /// Create semaphore
    void CreateSemaphore(std::string_view name, std::uint64_t limit);

    /// Update semaphore
    void UpdateSemaphore(std::string_view name, std::string_view data);

    /// Delete semaphore
    void DeleteSemaphore(std::string_view name);

private:
    NYdb::NCoordination::TSession session_;
};

/// @ingroup userver_clients
///
/// @brief YDB Coordination Client
///
/// Provides access to work with Coordination Service
/// @see https://ydb.tech/docs/ru/reference/ydb-sdk/coordination
class CoordinationClient final {
public:
    /// @cond
    // For internal use only.
    explicit CoordinationClient(std::shared_ptr<impl::Driver> driver);
    /// @endcond

    /// Start session
    /// @warning Use `TSessionSettings::OnStateChanged` and
    /// `TSessionSettings::OnStopped` callbacks with care, they will be executed
    /// on a non-coroutine thread
    CoordinationSession StartSession(std::string_view path, const NYdb::NCoordination::TSessionSettings& settings);

    /// Create coordination node
    void CreateNode(std::string_view path, const NYdb::NCoordination::TCreateNodeSettings& settings);

    /// Alter coordination node
    void AlterNode(std::string_view path, const NYdb::NCoordination::TAlterNodeSettings& settings);

    /// Drop coordination node
    void DropNode(std::string_view path);

    /// Describe coordination node
    NYdb::NCoordination::TNodeDescription DescribeNode(std::string_view path);

    /// Get native coordination client
    /// @warning Use with care! Facilities from
    /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
    /// non-blocking wait operations.
    NYdb::NCoordination::TClient& GetNativeCoordinationClient();

private:
    std::shared_ptr<impl::Driver> driver_;
    NYdb::NCoordination::TClient client_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
