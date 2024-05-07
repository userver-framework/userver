#pragma once

#include <memory>
#include <string_view>

#include <ydb-cpp-sdk/client/coordination/coordination.h>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
}  // namespace impl

class CoordinationSession final {
 public:
  /// @cond
  // For internal use only.
  explicit CoordinationSession(NYdb::NCoordination::TSession&& session);
  /// @endcond

  std::uint64_t GetSessionId();

  NYdb::NCoordination::ESessionState GetSessionState();

  NYdb::NCoordination::EConnectionState GetConnectionState();

  void Close();

  void Ping();

  void Reconnect();

  /// @warning Use `TAcquireSemaphoreSettings::OnAccepted` callback with care,
  /// it will be executed on a non-coroutine thread
  bool AcquireSemaphore(
      std::string_view name,
      const NYdb::NCoordination::TAcquireSemaphoreSettings& settings);

  bool ReleaseSemaphore(std::string_view name);

  /// @warning Use `TDescribeSemaphoreSettings::OnChanged` callback with care,
  /// it will be executed on a non-coroutine thread
  NYdb::NCoordination::TSemaphoreDescription DescribeSemaphore(
      std::string_view name,
      const NYdb::NCoordination::TDescribeSemaphoreSettings& settings);

  void CreateSemaphore(std::string_view name, std::uint64_t limit);

  void UpdateSemaphore(std::string_view name, std::string_view data);

  void DeleteSemaphore(std::string_view name);

 private:
  NYdb::NCoordination::TSession session_;
};

class CoordinationClient final {
 public:
  /// @cond
  // For internal use only.
  explicit CoordinationClient(std::shared_ptr<impl::Driver> driver);
  /// @endcond

  /// @warning Use `TSessionSettings::OnStateChanged` and
  /// `TSessionSettings::OnStopped` callbacks with care, they will be executed
  /// on a non-coroutine thread
  CoordinationSession StartSession(
      std::string_view path,
      const NYdb::NCoordination::TSessionSettings& settings);

  void CreateNode(std::string_view path,
                  const NYdb::NCoordination::TCreateNodeSettings& settings);

  void AlterNode(std::string_view path,
                 const NYdb::NCoordination::TAlterNodeSettings& settings);

  void DropNode(std::string_view path);

  NYdb::NCoordination::TNodeDescription DescribeNode(std::string_view path);

  NYdb::NCoordination::TClient& GetNativeCoordinationClient();

 private:
  std::shared_ptr<impl::Driver> driver_;
  NYdb::NCoordination::TClient client_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
