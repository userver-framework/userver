#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/urabbitmq/cluster.hpp>
#include <userver/urabbitmq/channel.hpp>
#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/consumer_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

class Consumer final : public urabbitmq::ConsumerBase {
 public:
  using urabbitmq::ConsumerBase::ConsumerBase;

  ~Consumer() override {
    Stop();
  }

  void Process(std::string message) override {}
};

}  // namespace

UTEST(We, We) {
  auto resolver = CreateResolver();
  const auto cluster = std::make_shared<urabbitmq::Cluster>(resolver);

  const urabbitmq::Exchange exchange{"userver-exchange"};
  const urabbitmq::Queue queue{"userver-queue"};
  const std::string routing_key{"userver-routing-key"};

  {
    auto admin = cluster->GetAdminChannel();
    admin.DeclareExchange(exchange, urabbitmq::ExchangeType::kFanOut);
    admin.DeclareQueue(queue);
    admin.BindQueue(exchange, queue, routing_key);
  }

  {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < 10; ++i) {
      tasks.emplace_back(
          engine::AsyncNoSpan([&cluster, &exchange, &routing_key, i] {
            cluster->GetChannel().PublishReliable(exchange, routing_key,
                                                  std::to_string(i));
          }));
    }

    engine::WaitAllChecked(tasks);
  }

  Consumer consumer{cluster, queue};
  consumer.Start();
}

USERVER_NAMESPACE_END
