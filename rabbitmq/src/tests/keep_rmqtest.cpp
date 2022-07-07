#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <urabbitmq/channel_pool.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>
#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/test_consumer_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

class Consumer final : public urabbitmq::TestConsumerBase {
 public:
  using urabbitmq::TestConsumerBase::TestConsumerBase;

  void Process(std::string message) override {}
};

}  // namespace

UTEST(We, We) {
  auto resolver = CreateResolver();
  auto ev = engine::current_task::GetEventThread();
  const auto address = AMQP::Address{"amqp://guest:guest@localhost/"};

  urabbitmq::impl::AmqpConnectionHandler handler{resolver, ev, address};
  urabbitmq::impl::AmqpConnection connection{handler};
  urabbitmq::impl::AmqpChannel channel{connection};
  urabbitmq::impl::AmqpReliableChannel reliable{connection};

  const urabbitmq::Exchange exchange{"userver-exchange"};
  const urabbitmq::Queue queue{"userver-queue"};
  const std::string routing_key{"userver-routing-key"};

  channel.DeclareExchange(exchange, urabbitmq::ExchangeType::kFanOut);
  channel.DeclareQueue(queue);
  channel.BindQueue(exchange, queue, routing_key);

  /*for (;;) {
    reliable.Publish(exchange, routing_key, "hi!");
    engine::SleepFor(std::chrono::milliseconds{20});
  }*/

  auto pool = std::make_shared<urabbitmq::ChannelPool>(
      connection, urabbitmq::ChannelPoolMode::kReliable);

  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < 10; ++i) {
    tasks.emplace_back(engine::AsyncNoSpan([&pool, &exchange, &routing_key, i] {
      auto channel = pool->Acquire();
      channel->Publish(exchange, routing_key, std::to_string(i));
    }));
  }

  engine::WaitAllChecked(tasks);

  Consumer consumer{channel, queue};
  consumer.Start();

  engine::SleepUntil({});
}

USERVER_NAMESPACE_END
