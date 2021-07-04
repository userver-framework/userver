#include <userver/storages/redis/impl/types.hpp>

#include <engine/blocking_future.hpp>
#include <userver/engine/task/cancel.hpp>

#include <userver/storages/redis/impl/exception.hpp>

namespace redis {

class ReplyPtrFutureImpl : public engine::impl::BlockingFuture<ReplyPtr> {
 public:
  ReplyPtrFutureImpl() = default;
  ReplyPtrFutureImpl(engine::impl::BlockingFuture<ReplyPtr>&& impl)
      : engine::impl::BlockingFuture<ReplyPtr>(std::move(impl)) {}
};

class ReplyPtrPromiseImpl : public engine::impl::BlockingPromise<ReplyPtr> {};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ReplyPtrFuture::ReplyPtrFuture() = default;

ReplyPtrFuture::ReplyPtrFuture(ReplyPtrFutureImpl&& impl)
    : impl_(std::move(impl)) {}

ReplyPtrFuture::~ReplyPtrFuture() = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ReplyPtrFuture::ReplyPtrFuture(ReplyPtrFuture&&) noexcept = default;
ReplyPtrFuture& ReplyPtrFuture::operator=(ReplyPtrFuture&&) noexcept = default;

std::future_status ReplyPtrFuture::wait_until(
    const std::chrono::steady_clock::time_point& until) const {
  auto result = impl_->wait_until(until);
  if (result != std::future_status::ready &&
      engine::current_task::ShouldCancel()) {
    throw RequestCancelledException(
        "Redis request wait was aborted due to task cancellation");
  }
  return result;
}

ReplyPtr ReplyPtrFuture::get() { return impl_->get(); }

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ReplyPtrPromise::ReplyPtrPromise() = default;
ReplyPtrPromise::~ReplyPtrPromise() = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ReplyPtrPromise::ReplyPtrPromise(ReplyPtrPromise&&) noexcept = default;
ReplyPtrPromise& ReplyPtrPromise::operator=(ReplyPtrPromise&&) noexcept =
    default;

ReplyPtrFuture ReplyPtrPromise::get_future() {
  return ReplyPtrFuture(impl_->get_future());
}

void ReplyPtrPromise::set_value(ReplyPtr reply) {
  impl_->set_value(std::move(reply));
}

}  // namespace redis
