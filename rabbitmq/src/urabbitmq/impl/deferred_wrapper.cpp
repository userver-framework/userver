#include "deferred_wrapper.hpp"

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

void DeferredWrapper::Fail(const char* message) {
  error_.emplace(message);
  event_.Send();
}

void DeferredWrapper::Ok() {
  event_.Send();
}

void DeferredWrapper::Wait(engine::Deadline deadline) {
  if (!event_.WaitForEventUntil(deadline)) {
    throw std::runtime_error{"operation timeout"};
  }

  if (error_.has_value()) {
    throw std::runtime_error{*error_};
  }
}

std::shared_ptr<DeferredWrapper> DeferredWrapper::Create() {
  return std::make_shared<DeferredWrapper>();
}

void DeferredWrapper::Wrap(AMQP::Deferred& deferred) {
  deferred
      .onSuccess([wrap = shared_from_this()] { wrap->Ok();})
      .onError([wrap = shared_from_this()] (const char* error) { wrap->Fail(error); });
}

}

USERVER_NAMESPACE_END