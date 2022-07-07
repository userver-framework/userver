#include "deferred_wrapper.hpp"

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

void DeferredWrapper::Fail(const char* message) {
  error.emplace(message);
  event.Send();
}

void DeferredWrapper::Ok() {
  event.Send();
}

void DeferredWrapper::Wait(engine::Deadline deadline) {
  if (!event.WaitForEventUntil(deadline)) {
    throw std::runtime_error{"operation timeout"};
  }

  if (error.has_value()) {
    throw std::runtime_error{*error};
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