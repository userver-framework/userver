#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include "utils_rmqtest.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class Consumer final : public urabbitmq::ConsumerBase {
 public:
  using urabbitmq::ConsumerBase::ConsumerBase;

  ~Consumer() override { Stop(); }

  void Process([[maybe_unused]] std::string message) override {
    // engine::InterruptibleSleepFor(std::chrono::milliseconds{100});
    //  throw std::runtime_error{message};
  }
};

}  // namespace

// TODO : remove this before merging
// There are plenty of cases that require manual intervention
// (dropping connection/stopping RabbitMQ gracefully/killing RMQ process etc.),
// and i use some combinations of this test to check them
UTEST_MT(We, DISABLED_We, 3) {
  ClientWrapper client{};

  const urabbitmq::Exchange exchange{"userver-exchange"};
  const urabbitmq::Queue queue{"userver-queue"};
  const std::string routing_key{"userver-routing-key"};

  /*{
    auto admin = client->GetAdminChannel(client.GetDeadline());
    admin.DeclareExchange(exchange, urabbitmq::Exchange::Type::kFanOut, {},
                          client.GetDeadline());
    admin.DeclareQueue(queue, urabbitmq::Queue::Flags::kDurable,
                       client.GetDeadline());
    admin.BindQueue(exchange, queue, routing_key, client.GetDeadline());
  }*/

  /*auto channel = client->GetChannel();
  while (true) {
    channel.Publish(exchange, routing_key, "hi!",
  urabbitmq::MessageType::kTransient,
                    {});
  }*/

  /*{
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < 3; ++i) {
      tasks.emplace_back(
          engine::AsyncNoSpan([&client, &exchange, &routing_key, i] {
            client->GetReliableChannel(client.GetDeadline())
                .PublishReliable(exchange, routing_key, std::to_string(i),
                                 urabbitmq::MessageType::kTransient,
                                 client.GetDeadline());
          }));
    }

    engine::WaitAllChecked(tasks);
  }*/

  // const auto stats = client->GetStatistics();
  // EXPECT_EQ(formats::json::ToString(stats), "");

  /*const auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < 1000; ++i) {
    client->PublishReliable(exchange, routing_key, "message",
  urabbitmq::MessageType::kTransient, {});
  }
  const auto finish = std::chrono::steady_clock::now();

  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(finish -
  start).count(), 1);*/

  bool publish = false;
  if (publish) {
    try {
      std::vector<engine::TaskWithResult<void>> publishers;
      for (size_t k = 0; k < 30; ++k) {
        publishers.emplace_back(
            engine::AsyncNoSpan([&client, &exchange, &routing_key] {
              // auto channel =
              // client->GetReliableChannel(client.GetDeadline());
              const std::string rmq_message(1 << 4, 'a');
              for (size_t i = 0; !engine::current_task::ShouldCancel(); ++i) {
                client->PublishReliable(exchange, routing_key, rmq_message, {});
              }
            }));
      }
      // engine::WaitAllChecked(publishers);
      for (auto& task : publishers) {
        task.Wait();
      }
      EXPECT_EQ(formats::json::ToString(client->GetStatistics()), "");
    } catch (const std::exception&) {
      EXPECT_EQ(formats::json::ToString(client->GetStatistics()), "");
    }
  } else {
    const urabbitmq::ConsumerSettings settings{queue, 33};
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
