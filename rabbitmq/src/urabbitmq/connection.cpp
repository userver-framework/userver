#include "connection.hpp"

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utils/assert.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Connection::Connection(clients::dns::Resolver& resolver,
                       engine::ev::ThreadControl& thread,
                       const ConnectionSettings& settings,
                       const EndpointInfo& endpoint,
                       const AuthSettings& auth_settings)
    : handler_{resolver, thread, endpoint, auth_settings},
      conn_{handler_},
      settings_{settings},
      queue_{settings.max_channels} {
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(settings.max_channels);
  for (size_t i = 0; i < settings.max_channels; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this] { AddChannel(); }));
  }
  engine::WaitAllChecked(init_tasks);

  size_monitor_.Start(
      "channels_count_monitor", {std::chrono::milliseconds{1000}}, [this] {
        if (size_.load(std::memory_order_relaxed) +
                given_away_.load(std::memory_order_relaxed) <
            settings_.max_channels) {
          try {
            AddChannel();
          } catch (const std::exception& ex) {
            LOG_WARNING() << "Failed to add channel: '" << ex.what() << "'";
          }
        }
      });
}

Connection::~Connection() {
  size_monitor_.Stop();

  handler_.GetEvThread().RunInEvLoopSync([this] {
    while (true) {
      auto* ptr = TryPop();
      if (!ptr) break;

      Drop(ptr);
    }
  });
}

ChannelPtr Connection::Acquire() { return {shared_from_this(), Pop()}; }

void Connection::Release(impl::IAmqpChannel* channel) noexcept {
  UASSERT(channel);

  channel->ResetCallbacks();

  if (!queue_.bounded_push(channel)) {
    Drop(channel);
  }
  given_away_.fetch_add(-1, std::memory_order_relaxed);
  size_.fetch_add(1, std::memory_order_relaxed);
}

bool Connection::IsBroken() const { return handler_.IsBroken(); }

void Connection::NotifyChannelAdopted() {
  given_away_.fetch_add(-1, std::memory_order_relaxed);
}

impl::IAmqpChannel* Connection::Pop() {
  auto* ptr = TryPop();

  if (!ptr) {
    // TODO : fix me
    throw std::runtime_error{"oh well"};
  }
  given_away_.fetch_add(1, std::memory_order_relaxed);
  size_.fetch_add(-1, std::memory_order_relaxed);
  return ptr;
}

impl::IAmqpChannel* Connection::TryPop() {
  impl::IAmqpChannel* ptr{nullptr};
  if (queue_.pop(ptr)) {
    return ptr;
  }

  return nullptr;
}

std::unique_ptr<impl::IAmqpChannel> Connection::CreateChannel() {
  switch (settings_.mode) {
    case ConnectionMode::kUnreliable:
      return std::make_unique<impl::AmqpChannel>(conn_);
    case ConnectionMode::kReliable:
      return std::make_unique<impl::AmqpReliableChannel>(conn_);
  }
}

void Connection::Drop(impl::IAmqpChannel* channel) {
  std::default_delete<impl::IAmqpChannel>{}(channel);
  size_.fetch_add(-1, std::memory_order_relaxed);
}

void Connection::AddChannel() {
  auto* ptr = CreateChannel().release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
  size_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
