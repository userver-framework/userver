#pragma once

#include <future>
#include <memory>
#include <type_traits>

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

class ResponseFuture final {
 public:
  ResponseFuture(engine::Future<std::shared_ptr<Response>>&& future,
                 std::chrono::milliseconds total_timeout,
                 std::shared_ptr<RequestState> request);

  ResponseFuture(ResponseFuture&& other) noexcept;

  ResponseFuture(const ResponseFuture&) = delete;

  ~ResponseFuture();

  ResponseFuture& operator=(const ResponseFuture&) = delete;

  ResponseFuture& operator=(ResponseFuture&&) noexcept;

  void Cancel();

  void Detach();

  std::future_status Wait();

  std::shared_ptr<Response> Get();

  /// @cond
  /// Internal helper for WaitAny/WaitAll
  engine::impl::ContextAccessor* TryGetContextAccessor() noexcept;
  /// @endcond

 private:
  engine::Future<std::shared_ptr<Response>> future_;
  engine::Deadline deadline_;
  std::shared_ptr<RequestState> request_state_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
