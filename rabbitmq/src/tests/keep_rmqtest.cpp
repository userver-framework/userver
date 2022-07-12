#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/rabbitmq.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

std::shared_ptr<urabbitmq::Client> CreateClient(
    userver::clients::dns::Resolver& resolver) {
  urabbitmq::AuthSettings auth{};
  urabbitmq::EndpointInfo endpoint{};

  const urabbitmq::ClientSettings settings{urabbitmq::EvPoolType::kOwned,
                                           2,
                                           1,
                                           10,
                                           {std::move(endpoint)},
                                           std::move(auth)};

  return std::make_shared<urabbitmq::Client>(resolver, settings);
}

class Consumer final : public urabbitmq::ConsumerBase {
 public:
  using urabbitmq::ConsumerBase::ConsumerBase;

  ~Consumer() override { Stop(); }

  void Process(std::string message) override {}
};

class ClientWrapper final {
 public:
  ClientWrapper()
      : resolver_{CreateResolver()}, client_{CreateClient(resolver_)} {}

  urabbitmq::Client& operator*() const { return *client_; }

  urabbitmq::Client* operator->() const { return client_.get(); }

  std::shared_ptr<urabbitmq::Client> Get() const { return client_; }

 private:
  userver::clients::dns::Resolver resolver_;
  std::shared_ptr<urabbitmq::Client> client_;
};

}  // namespace

UTEST(We, We) {
  ClientWrapper client{};

  const urabbitmq::Exchange exchange{"userver-exchange"};
  const urabbitmq::Queue queue{"userver-queue"};
  const std::string routing_key{"userver-routing-key"};

  {
    auto admin = client->GetAdminChannel();
    admin.DeclareExchange(exchange, urabbitmq::ExchangeType::kFanOut);
    admin.DeclareQueue(queue);
    admin.BindQueue(exchange, queue, routing_key);
  }

  {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < 10; ++i) {
      tasks.emplace_back(
          engine::AsyncNoSpan([&client, &exchange, &routing_key, i] {
            client->GetChannel().PublishReliable(exchange, routing_key,
                                                 std::to_string(i));
          }));
    }

    engine::WaitAllChecked(tasks);
  }

  bool publish = false;

  if (publish) {
    std::vector<engine::TaskWithResult<void>> publishers;
    for (size_t i = 0; i < 3; ++i) {
      publishers.emplace_back(
          engine::AsyncNoSpan([&client, &exchange, &routing_key] {
            for (size_t i = 0; !engine::current_task::ShouldCancel(); ++i) {
              client->GetChannel().Publish(exchange, routing_key,
                                           std::to_string(i));
            }
          }));
    }
    engine::SleepUntil({});
  } else {
    const urabbitmq::ConsumerSettings settings{queue, 2000};
    Consumer consumer1{client.Get(), settings};
    Consumer consumer2{client.Get(), settings};
    Consumer consumer3{client.Get(), settings};
    consumer1.Start();
    consumer2.Start();
    consumer3.Start();

    engine::SleepUntil({});
  }
  // engine::WaitAllChecked(publishers);
}

USERVER_NAMESPACE_END
