#include "utils_rmqtest.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(AdminChannel, DeclareRemoveExchange) {
  ClientWrapper client{};
  auto channel = client->GetAdminChannel();
  urabbitmq::Exchange exchange{"some_exchange"};

  const auto declare_exchange = [&client, &exchange](
                                    urabbitmq::AdminChannel& channel,
                                    urabbitmq::ExchangeType type) {
    channel.DeclareExchange(exchange, type, {}, client.GetDeadline());
  };
  declare_exchange(channel, urabbitmq::ExchangeType::kFanOut);
  // 406 PRECONDITION_FAILED
  EXPECT_ANY_THROW(declare_exchange(channel, urabbitmq::ExchangeType::kDirect));

  // channel is broken
  EXPECT_ANY_THROW(channel.RemoveExchange(exchange, client.GetDeadline()));

  auto new_channel = client->GetAdminChannel();
  new_channel.RemoveExchange(exchange, client.GetDeadline());
  declare_exchange(new_channel, urabbitmq::ExchangeType::kFanOut);
  new_channel.RemoveExchange(exchange, client.GetDeadline());
}

UTEST(AdminChannel, DeclareRemoveQueue) {
  ClientWrapper client{};
  auto channel = client->GetAdminChannel();
  urabbitmq::Queue queue{"some_queue"};

  const auto declare_queue = [&client, &queue](
                                 urabbitmq::AdminChannel& channel,
                                 utils::Flags<urabbitmq::Queue::Flags> flags) {
    channel.DeclareQueue(queue, flags, client.GetDeadline());
  };
  declare_queue(channel, urabbitmq::Queue::Flags::kDurable);
  // 406 PRECONDITION_FAILED
  EXPECT_ANY_THROW(declare_queue(channel, urabbitmq::Queue::Flags::kNone));

  // channel is broken
  EXPECT_ANY_THROW(channel.RemoveQueue(queue, client.GetDeadline()));

  auto new_channel = client->GetAdminChannel();
  new_channel.RemoveQueue(queue, client.GetDeadline());
  declare_queue(new_channel, urabbitmq::Queue::Flags::kNone);
  new_channel.RemoveQueue(queue, client.GetDeadline());
}

USERVER_NAMESPACE_END
