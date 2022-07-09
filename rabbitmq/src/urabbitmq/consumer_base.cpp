#include <userver/urabbitmq/consumer_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/client.hpp>

#include <urabbitmq/client_impl.hpp>
#include <urabbitmq/consumer_base_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

std::unique_ptr<ConsumerBaseImpl> CreateConsumerImpl(
    ClientImpl& client_impl, const ConsumerSettings& settings) {
  return std::make_unique<ConsumerBaseImpl>(client_impl.GetUnreliable(),
                                            settings);
}

}  // namespace

ConsumerBase::ConsumerBase(std::shared_ptr<Client> client,
                           const ConsumerSettings& settings)
    : client_{std::move(client)},
      settings_{settings},
      impl_{CreateConsumerImpl(*client_->impl_, settings_)} {}

ConsumerBase::~ConsumerBase() { Stop(); }

void ConsumerBase::Start() {
  impl_->Start(
      [this](std::string message) mutable { Process(std::move(message)); });

  monitor_.Start("consumer_monitor", {std::chrono::seconds{1}}, [this] {
    if (impl_->IsBroken()) {
      LOG_WARNING() << "Consumer for queue '" << settings_.queue.GetUnderlying()
                    << "' is broken, trying to restart";
      try {
        impl_ = CreateConsumerImpl(*client_->impl_, settings_);
        impl_->Start([this](std::string message) mutable {
          Process(std::move(message));
        });
        LOG_INFO() << "Restarted successfully";
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to restart a consumer: '" << ex.what()
                    << "'; will try to restart again";
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
