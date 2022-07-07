#include "test_consumer_base.hpp"

#include "consumer_base_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

TestConsumerBase::TestConsumerBase(impl::AmqpChannel& channel, const Queue& queue)
    : impl_{std::make_unique<ConsumerBaseImpl>(channel, queue.GetUnderlying())} {}

TestConsumerBase::~TestConsumerBase() {
  Stop();
}

void TestConsumerBase::Start() {
  impl_->Start([this] (std::string message) {
    Process(std::move(message));
  });
}

void TestConsumerBase::Stop() {
  impl_->Stop();
}

}

USERVER_NAMESPACE_END
