#include <userver/urabbitmq/consumer_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/urabbitmq/cluster.hpp>

#include <urabbitmq/cluster_impl.hpp>
#include <urabbitmq/consumer_base_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

std::unique_ptr<ConsumerBaseImpl> CreateConsumerImpl(ClusterImpl& cluster_impl,
                                                     const std::string& queue) {
  return std::make_unique<ConsumerBaseImpl>(cluster_impl.GetUnreliable(),
                                            queue);
}

}  // namespace

ConsumerBase::ConsumerBase(std::shared_ptr<Cluster> cluster, const Queue& queue)
    : cluster_{std::move(cluster)},
      queue_name_{queue.GetUnderlying()},
      impl_{CreateConsumerImpl(*cluster_->impl_, queue_name_)} {}

ConsumerBase::~ConsumerBase() { Stop(); }

void ConsumerBase::Start() {
  impl_->Start(
      [this](std::string message) mutable { Process(std::move(message)); });

  monitor_.Start("consumer_monitor", {std::chrono::seconds{1}}, [this] {
    if (impl_->IsBroken()) {
      try {
        impl_ = CreateConsumerImpl(*cluster_->impl_, queue_name_);
        impl_->Start([this](std::string message) mutable {
          Process(std::move(message));
        });
      } catch (const std::exception& e) {
        int a = 5;
        // TODO : log error
      }
    }
  });
}

void ConsumerBase::Stop() {
  monitor_.Stop();
  impl_->Stop();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
