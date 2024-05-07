#include <userver/ydb/coordination.hpp>

#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

template <typename T>
T ExtractResult(NThreading::TFuture<NYdb::NCoordination::TResult<T>>&& future,
                std::string_view operation_name) {
  return impl::GetFutureValueChecked(std::move(future), operation_name)
      .ExtractResult();
}

}  // namespace

CoordinationSession::CoordinationSession(
    NYdb::NCoordination::TSession&& session)
    : session_{std::move(session)} {
  UASSERT(session_);
}

std::uint64_t CoordinationSession::GetSessionId() {
  return session_.GetSessionId();
}

NYdb::NCoordination::ESessionState CoordinationSession::GetSessionState() {
  return session_.GetSessionState();
}

NYdb::NCoordination::EConnectionState
CoordinationSession::GetConnectionState() {
  return session_.GetConnectionState();
}

void CoordinationSession::Close() { ExtractResult(session_.Close(), "Close"); }

void CoordinationSession::Ping() { ExtractResult(session_.Ping(), "Ping"); }

void CoordinationSession::Reconnect() {
  ExtractResult(session_.Reconnect(), "Reconnect");
}

bool CoordinationSession::AcquireSemaphore(
    std::string_view name,
    const NYdb::NCoordination::TAcquireSemaphoreSettings& settings) {
  return ExtractResult(
      session_.AcquireSemaphore(impl::ToString(name), settings),
      "AcquireSemaphore");
}

bool CoordinationSession::ReleaseSemaphore(std::string_view name) {
  return ExtractResult(session_.ReleaseSemaphore(impl::ToString(name)),
                       "ReleaseSemaphore");
}

NYdb::NCoordination::TSemaphoreDescription
CoordinationSession::DescribeSemaphore(
    std::string_view name,
    const NYdb::NCoordination::TDescribeSemaphoreSettings& settings) {
  return ExtractResult(
      session_.DescribeSemaphore(impl::ToString(name), settings),
      "DescribeSemaphore");
}

void CoordinationSession::CreateSemaphore(std::string_view name,
                                          std::uint64_t limit) {
  ExtractResult(session_.CreateSemaphore(impl::ToString(name), limit),
                "CreateSemaphore");
}

void CoordinationSession::UpdateSemaphore(std::string_view name,
                                          std::string_view data) {
  ExtractResult(
      session_.UpdateSemaphore(impl::ToString(name), impl::ToString(data)),
      "UpdateSemaphore");
}

void CoordinationSession::DeleteSemaphore(std::string_view name) {
  ExtractResult(session_.DeleteSemaphore(impl::ToString(name)),
                "DeleteSemaphore");
}

CoordinationClient::CoordinationClient(std::shared_ptr<impl::Driver> driver)
    : driver_{std::move(driver)}, client_{driver_->GetNativeDriver()} {}

CoordinationSession CoordinationClient::StartSession(
    std::string_view path,
    const NYdb::NCoordination::TSessionSettings& settings) {
  auto session =
      ExtractResult(client_.StartSession(
                        impl::JoinPath(driver_->GetDbPath(), path), settings),
                    "StartSession");
  return CoordinationSession{std::move(session)};
}

void CoordinationClient::CreateNode(
    std::string_view path,
    const NYdb::NCoordination::TCreateNodeSettings& settings) {
  impl::GetFutureValueChecked(
      client_.CreateNode(impl::JoinPath(driver_->GetDbPath(), path), settings),
      "CreateNode");
}

void CoordinationClient::AlterNode(
    std::string_view path,
    const NYdb::NCoordination::TAlterNodeSettings& settings) {
  impl::GetFutureValueChecked(
      client_.AlterNode(impl::JoinPath(driver_->GetDbPath(), path), settings),
      "AlterNode");
}

void CoordinationClient::DropNode(std::string_view path) {
  impl::GetFutureValueChecked(
      client_.DropNode(impl::JoinPath(driver_->GetDbPath(), path)), "DropNode");
}

NYdb::NCoordination::TNodeDescription CoordinationClient::DescribeNode(
    std::string_view path) {
  return ExtractResult(
      client_.DescribeNode(impl::JoinPath(driver_->GetDbPath(), path)),
      "DescribeNode");
}

NYdb::NCoordination::TClient&
CoordinationClient::GetNativeCoordinationClient() {
  return client_;
}

}  // namespace ydb

USERVER_NAMESPACE_END
