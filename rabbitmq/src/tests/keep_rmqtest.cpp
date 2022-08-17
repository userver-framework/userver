#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utils/scope_guard.hpp>

#include "utils_rmqtest.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class Consumer final : public urabbitmq::ConsumerBase {
 public:
  using urabbitmq::ConsumerBase::ConsumerBase;

  ~Consumer() override { Stop(); }

  void Process(std::string message) override {
    engine::InterruptibleSleepFor(std::chrono::milliseconds{100});
    // throw std::runtime_error{message};
  }
};

}  // namespace

UTEST_MT(We, We, 1) {
  ClientWrapper client{};

  const urabbitmq::Exchange exchange{"userver-exchange"};
  const urabbitmq::Queue queue{"userver-queue"};
  const std::string routing_key{"userver-routing-key"};

  {
    auto admin = client->GetAdminChannel();
    admin.DeclareExchange(exchange, urabbitmq::Exchange::Type::kFanOut, {},
                          client.GetDeadline());
    admin.DeclareQueue(queue, urabbitmq::Queue::Flags::kDurable,
                       client.GetDeadline());
    admin.BindQueue(exchange, queue, routing_key, client.GetDeadline());
  }

  /*auto channel = client->GetChannel();
  while (true) {
    channel.Publish(exchange, routing_key, "hi!",
  urabbitmq::MessageType::kTransient,
                    {});
  }*/

  {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < 1; ++i) {
      tasks.emplace_back(
          engine::AsyncNoSpan([&client, &exchange, &routing_key, i] {
            client->GetReliableChannel().Publish(
                exchange, routing_key, std::to_string(i),
                urabbitmq::MessageType::kTransient, client.GetDeadline());
          }));
    }

    engine::WaitAllChecked(tasks);
  }

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
  return;
  if (publish) {
    try {
      std::vector<engine::TaskWithResult<void>> publishers;
      for (size_t k = 0; k < 20; ++k) {
        publishers.emplace_back(
            engine::AsyncNoSpan([&client, &exchange, &routing_key] {
              auto channel = client->GetReliableChannel();
              const std::string rmq_message(1 << 4, 'a');
              for (size_t i = 0; !engine::current_task::ShouldCancel(); ++i) {
                channel.Publish(exchange, routing_key, rmq_message,
                                urabbitmq::MessageType::kPersistent, {});
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
    const urabbitmq::ConsumerSettings settings{queue, 10};
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
