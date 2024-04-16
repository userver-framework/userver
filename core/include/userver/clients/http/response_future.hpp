#pragma once

/// @file userver/clients/http/response_future.hpp
/// @brief @copybrief clients::http::ResponseFuture

#include <future>
#include <memory>
#include <type_traits>

#include <userver/clients/http/config.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/compiler/select.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/impl/context_accessor.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class RequestState;

namespace impl {
class EasyWrapper;
}  // namespace impl

/// @brief Allows to perform a request concurrently with other work without
/// creating an extra coroutine for waiting.
class ResponseFuture final {
 public:
  ResponseFuture(ResponseFuture&& other) noexcept;
  ResponseFuture& operator=(ResponseFuture&&) noexcept;
  ResponseFuture(const ResponseFuture&) = delete;
  ResponseFuture& operator=(const ResponseFuture&) = delete;
  ~ResponseFuture();

  void Cancel();

  void Detach();

  std::future_status Wait();

  std::shared_ptr<Response> Get();

  void SetCancellationPolicy(CancellationPolicy cp);

  /// @cond
  /// Internal helper for WaitAny/WaitAll
  engine::impl::ContextAccessor* TryGetContextAccessor() noexcept;

  ResponseFuture(engine::Future<std::shared_ptr<Response>>&& future,
                 std::shared_ptr<RequestState> request);
  /// @endcond

 private:
  void CancelOrDetach();

  engine::Future<std::shared_ptr<Response>> future_;
  engine::Deadline deadline_;
  std::shared_ptr<RequestState> request_state_;
  bool was_deadline_propagated_{false};
  CancellationPolicy cancellation_policy_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
