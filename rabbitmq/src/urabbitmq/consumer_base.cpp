#include <userver/urabbitmq/consumer_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/client.hpp>

#include <urabbitmq/client_impl.hpp>
#include <urabbitmq/consumer_base_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

constexpr std::chrono::milliseconds kConnectionAcquisitionTimeout{1000};
constexpr std::chrono::seconds kMonitorInterval{1};

template <typename OnMessage>
std::unique_ptr<ConsumerBaseImpl> CreateAndStartConsumerImpl(
    ClientImpl& client_impl, const ConsumerSettings& settings,
    OnMessage&& on_message) {
  auto impl = std::make_unique<ConsumerBaseImpl>(
      client_impl.GetConnection(
          engine::Deadline::FromDuration(kConnectionAcquisitionTimeout)),
      settings);
  impl->Start(std::forward<OnMessage>(on_message));

  return impl;
}

}  // namespace

ConsumerBase::ConsumerBase(std::shared_ptr<Client> client,
                           const ConsumerSettings& settings)
    : client_{std::move(client)}, settings_{settings}, impl_{nullptr} {
  UASSERT(client_);
}

ConsumerBase::~ConsumerBase() {
  UASSERT_MSG(impl_ == nullptr,
              "You should call `Stop` before derived class is destroyed");
  Stop();
}

void ConsumerBase::Start() {
  if (monitor_.IsRunning()) {
    return;
  }

  try {
    impl_ = CreateAndStartConsumerImpl(
        *client_->impl_, settings_,
        [this](std::string message) { Process(std::move(message)); });
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to start a consumer: '" << ex.what()
                  << "'; will try to start again";
  }

  monitor_.Start(
      fmt::format("{}_consumer_monitor", settings_.queue.GetUnderlying()),
      {kMonitorInterval}, [this] {
        if (impl_ == nullptr || impl_->IsBroken()) {
          LOG_WARNING() << "Consumer for queue '"
                        << settings_.queue.GetUnderlying()
                        << "' is broken, trying to restart";
          try {
            // TODO : there is a subtle problem with this:
            // we might set up all the consumers over the same host if some
            // nodes fail or we are just unlucky. Not sure how much of a problem
            // that is, but still
            impl_.reset();
            impl_ = CreateAndStartConsumerImpl(
                *client_->impl_, settings_,
                [this](std::string message) { Process(std::move(message)); });
            LOG_INFO() << "Restarted successfully";
          } catch (const std::exception& ex) {
            LOG_WARNING() << "Failed to restart a consumer: '" << ex.what()
                          << "'; will try to restart again";
          }
        }
      });
}

void ConsumerBase::Stop() {
  monitor_.Stop();
  impl_.reset();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
