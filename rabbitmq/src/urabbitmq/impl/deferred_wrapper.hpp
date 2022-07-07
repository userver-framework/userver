#pragma once

#include <memory>
#include <optional>
#include <string>

#include <userver/engine/single_consumer_event.hpp>

namespace AMQP {
class Deferred;
}

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class DeferredWrapper : public std::enable_shared_from_this<DeferredWrapper> {
 public:
  void Fail(const char* message);

  void Ok();

  void Wait(engine::Deadline deadline = {});

  static std::shared_ptr<DeferredWrapper> Create();

  void Wrap(AMQP::Deferred& deferred);

 private:
  engine::SingleConsumerEvent event;
  std::optional<std::string> error;
};

}

USERVER_NAMESPACE_END