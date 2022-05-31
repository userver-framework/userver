#pragma once

#include <future>
#include <memory>
#include <type_traits>

#include <userver/clients/http/response.hpp>
#include <userver/compiler/select.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
template <typename T>
class BlockingFuture;
}  // namespace engine::impl

namespace clients::http {

class RequestState;

namespace impl {
class EasyWrapper;
}  // namespace impl

class ResponseFuture final {
 public:
  ResponseFuture(
      engine::impl::BlockingFuture<std::shared_ptr<Response>>&& future,
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
  static constexpr std::size_t kFutureSize = compiler::SelectSize()  //
                                                 .For64Bit(16)
                                                 .For32Bit(8);
  static constexpr std::size_t kFutureAlignment = alignof(void*);
  utils::FastPimpl<engine::impl::BlockingFuture<std::shared_ptr<Response>>,
                   kFutureSize, kFutureAlignment, utils::kStrictMatch>
      future_;
  engine::Deadline deadline_;
  std::shared_ptr<RequestState> request_state_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
