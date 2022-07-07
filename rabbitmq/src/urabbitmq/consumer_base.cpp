#include <userver/urabbitmq/consumer_base.hpp>

#include <userver/urabbitmq/cluster.hpp>

#include <urabbitmq/consumer_base_impl.hpp>
#include <urabbitmq/cluster_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ConsumerBase::ConsumerBase(std::shared_ptr<Cluster> cluster,
                           const Queue& queue)
  : cluster_{std::move(cluster)},
    impl_{std::make_unique<ConsumerBaseImpl>(cluster_->impl_->GetUnreliable(), queue.GetUnderlying())}
{}

ConsumerBase::~ConsumerBase() {
  Stop();
}

void ConsumerBase::Start() {
  impl_->Start([this](std::string message) mutable {
    Process(std::move(message));
  });
}

void ConsumerBase::Stop() {
  impl_->Stop();
}

}

USERVER_NAMESPACE_END
