#include "response_awaiter.hpp"

#ifndef NDEBUG
#include <userver/utils/assert.hpp>
#endif

#include <urabbitmq/impl/deferred_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

ResponseAwaiter::ResponseAwaiter(engine::SemaphoreLock&& lock)
    : lock_{std::move(lock)}, wrapper_{DeferredWrapper::Create()} {}

#ifndef NDEBUG
ResponseAwaiter::~ResponseAwaiter() {
  UASSERT_MSG(awaited_,
              "ResponseAwaiter dropped without waiting, shouldn't happen");
}
#else
ResponseAwaiter::~ResponseAwaiter() = default;
#endif

ResponseAwaiter::ResponseAwaiter(ResponseAwaiter&& other) noexcept = default;

void ResponseAwaiter::SetSpan(tracing::Span&& span) {
  span_.emplace(std::move(span));
}

void ResponseAwaiter::Wait(engine::Deadline deadline) const {
#ifndef NDEBUG
  awaited_ = true;
#endif
  GetWrapper()->Wait(deadline);
}

const std::shared_ptr<DeferredWrapper>& ResponseAwaiter::GetWrapper() const {
  return wrapper_;
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
