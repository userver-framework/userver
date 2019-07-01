#pragma once

#include <future>
#include <memory>
#include <type_traits>

#include <clients/http/response.hpp>
#include <utils/fast_pimpl.hpp>

namespace engine {

template <typename T>
class Future;
}  // namespace engine

namespace clients {
namespace http {

class ResponseFuture final {
 public:
  ResponseFuture(engine::Future<std::shared_ptr<Response>>&& future,
                 std::chrono::milliseconds total_timeout,
                 std::shared_ptr<EasyWrapper> easy);

  ResponseFuture(ResponseFuture&& other) noexcept;

  ResponseFuture(const ResponseFuture&) = delete;

  ~ResponseFuture();

  ResponseFuture& operator=(const ResponseFuture&) = delete;

  ResponseFuture& operator=(ResponseFuture&&) noexcept;

  void Cancel();

  void Detach();

  std::future_status Wait() const;

  std::shared_ptr<Response> Get();

 private:
  static constexpr auto kFutureSize = 16;
  static constexpr auto kFutureAlignment = 8;
  utils::FastPimpl<engine::Future<std::shared_ptr<Response>>, kFutureSize,
                   kFutureAlignment, true>
      future_;
  std::chrono::system_clock::time_point deadline_;
  std::shared_ptr<EasyWrapper> easy_;
};

}  // namespace http
}  // namespace clients
