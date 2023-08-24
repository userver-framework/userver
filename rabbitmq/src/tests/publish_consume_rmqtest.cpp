#include "utils_rmqtest.hpp"

#include <optional>

#include <userver/engine/sleep.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Consumer final : public urabbitmq::ConsumerBase {
 public:
  using urabbitmq::ConsumerBase::ConsumerBase;
  ~Consumer() override { Stop(); }

  void Process(std::string message) override {
    {
      auto locked = messages_.Lock();
      locked->emplace_back(std::move(message));
    }

    if (++consumed_ == expected_consumed_) {
      event_.Send();
    }
  }

  void ExpectConsume(size_t count) { expected_consumed_ = count; }

  std::vector<std::string> Wait() {
    if (expected_consumed_ != 0) {
      [[maybe_unused]] auto res =
          event_.WaitForEventFor(utest::kMaxTestWaitTime);
    }

    return Get();
  }

  std::vector<std::string> Get() {
    auto locked = messages_.Lock();
    return *locked;
  }

 private:
  std::optional<std::string> single_expected_message_;
  concurrent::Variable<std::vector<std::string>> messages_;
  std::atomic<size_t> expected_consumed_{0};
  std::atomic<size_t> consumed_{0};
  engine::SingleConsumerEvent event_;
};

class ThrowingConsumer final : public urabbitmq::ConsumerBase {
 public:
  using urabbitmq::ConsumerBase::ConsumerBase;
  ~ThrowingConsumer() override { Stop(); }

  void Process(std::string message) override {
    std::unique_lock<engine::Mutex> lock{mutex_};
    if (!thrown_) {
      cond_.WaitFor(lock, utest::kMaxTestWaitTime);
    }

    throw std::runtime_error{message};
  }

  void Throw() {
    std::unique_lock<engine::Mutex> lock{mutex_};
    thrown_ = true;
    cond_.NotifyAll();
  }

 private:
  engine::Mutex mutex_;
  bool thrown_{false};
  engine::ConditionVariable cond_;
};

}  // namespace

UTEST(Consumer, CreateOnInvalidQueueWorks) {
  ClientWrapper client{};
  const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

  Consumer consumer{client.Get(), settings};
}

UTEST(Consumer, CreateOnInvalidQueueStartStopWorks) {
  ClientWrapper client{};
  const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

  Consumer consumer{client.Get(), settings};
  consumer.Start();
  consumer.Stop();
}

UTEST(Consumer, ConsumeWorks) {
  ClientWrapper client{};
  client.SetupRmqEntities();
  const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

  const std::string message = "Hi from userver!";
  client->PublishReliable(client.GetExchange(), client.GetRoutingKey(), message,
                          urabbitmq::MessageType::kTransient,
                          client.GetDeadline());

  Consumer consumer{client.Get(), settings};
  consumer.ExpectConsume(1);

  consumer.Start();
  auto consumed = consumer.Wait();

  ASSERT_EQ(consumed.size(), 1);
  EXPECT_EQ(consumed[0], message);
}

UTEST(Consumer, ExhaustesQueue) {
  ClientWrapper client{};
  client.SetupRmqEntities();
  const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

  const size_t messages_count = 1000;
  for (size_t i = 0; i < messages_count; ++i) {
    auto channel = client->GetReliableChannel(client.GetDeadline());
    channel.PublishReliable(
        client.GetExchange(), client.GetRoutingKey(), std::to_string(i),
        urabbitmq::MessageType::kTransient, client.GetDeadline());
  }

  Consumer consumer{client.Get(), settings};
  consumer.ExpectConsume(messages_count);
  consumer.Start();

  consumer.Wait();
}

UTEST(Consumer, ThrowsReturnsToQueue) {
  ClientWrapper client{};
  client.SetupRmqEntities();
  const urabbitmq::ConsumerSettings settings{client.GetQueue(), 20};

  const size_t messages_count = 200;
  for (size_t i = 0; i < messages_count; ++i) {
    auto channel = client->GetReliableChannel(client.GetDeadline());
    channel.PublishReliable(
        client.GetExchange(), client.GetRoutingKey(), std::to_string(i),
        urabbitmq::MessageType::kTransient, client.GetDeadline());
  }

  ThrowingConsumer throwing_consumer{client.Get(), settings};
  Consumer good_consumer{client.Get(), settings};
  good_consumer.ExpectConsume(messages_count);
  throwing_consumer.Start();
  good_consumer.Start();
  engine::InterruptibleSleepFor(std::chrono::milliseconds{200});

  auto consumed = good_consumer.Get();
  EXPECT_LT(consumed.size(), messages_count);

  throwing_consumer.Throw();
  throwing_consumer.Stop();
  EXPECT_EQ(good_consumer.Wait().size(), messages_count);
}

UTEST(Consumer, MultipleConcurrentWork) {
  ClientWrapper client{};
  client.SetupRmqEntities();
  const urabbitmq::ConsumerSettings settings{client.GetQueue(), 20};

  const size_t messages_count = 1000;
  for (size_t i = 0; i < messages_count; ++i) {
    auto channel = client->GetReliableChannel(client.GetDeadline());
    channel.PublishReliable(
        client.GetExchange(), client.GetRoutingKey(), std::to_string(i),
        urabbitmq::MessageType::kTransient, client.GetDeadline());
  }

  Consumer first_consumer{client.Get(), settings};
  Consumer second_consumer{client.Get(), settings};
  first_consumer.Start();
  second_consumer.Start();

  engine::InterruptibleSleepFor(std::chrono::milliseconds{200});
  EXPECT_GT(first_consumer.Get().size(), 0);
  EXPECT_GT(second_consumer.Get().size(), 0);
}

UTEST(Consumer, ForDifferentQueuesWork) {
  ClientWrapper client{};
  client.SetupRmqEntities();

  const urabbitmq::Queue second_queue{utils::generators::GenerateUuid()};
  {
    auto channel = client->GetAdminChannel(client.GetDeadline());
    channel.DeclareQueue(second_queue, {}, client.GetDeadline());
    channel.BindQueue(client.GetExchange(), second_queue,
                      client.GetRoutingKey(), client.GetDeadline());
  }

  const size_t messages_count = 200;
  for (size_t i = 0; i < messages_count; ++i) {
    client->PublishReliable(
        client.GetExchange(), client.GetRoutingKey(), std::to_string(i),
        urabbitmq::MessageType::kTransient, client.GetDeadline());
  }

  Consumer first_consumer{client.Get(), {client.GetQueue(), 10}};
  Consumer second_consumer{client.Get(), {second_queue, 10}};
  first_consumer.ExpectConsume(messages_count);
  first_consumer.Start();
  second_consumer.ExpectConsume(messages_count);
  second_consumer.Start();

  EXPECT_EQ(first_consumer.Wait().size(), messages_count);
  EXPECT_EQ(second_consumer.Wait().size(), messages_count);

  client->GetAdminChannel(client.GetDeadline())
      .RemoveQueue(second_queue, client.GetDeadline());
}

USERVER_NAMESPACE_END
