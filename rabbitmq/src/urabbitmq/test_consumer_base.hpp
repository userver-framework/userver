#pragma once

#include <memory>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class AmqpChannel;
}

class ConsumerBaseImpl;

class TestConsumerBase {
 public:
  TestConsumerBase(impl::AmqpChannel& channel, const Queue& queue);
  virtual ~TestConsumerBase();

  void Start();
  void Stop();

  virtual void Process(std::string message) = 0;

 private:
  std::unique_ptr<ConsumerBaseImpl> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
