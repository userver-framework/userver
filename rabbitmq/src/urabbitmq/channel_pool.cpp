#include "channel_pool.hpp"

#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

constexpr std::chrono::milliseconds kChannelCreateTimeout{2000};

}

ChannelPool::ChannelPool(impl::AmqpConnectionHandler& handler,
                         impl::AmqpConnection& connection, bool reliable,
                         size_t max_channels)
    : thread_{handler.GetEvThread()},
      connection_{&connection},
      reliable_{reliable},
      max_channels_{max_channels},
      queue_{max_channels_} {
  std::vector<engine::TaskWithResult<void>> init_tasks;
  for (size_t i = 0; i < max_channels_; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this] { AddChannel(); }));
  }
  engine::WaitAllChecked(init_tasks);

  size_monitor_.Start(
      "channels_count_monitor", {std::chrono::milliseconds{1000}}, [this] {
        if (size_.load(std::memory_order_relaxed) +
                given_away_.load(std::memory_order_relaxed) <
            max_channels_) {
          try {
            AddChannel();
          } catch (const std::exception& ex) {
            LOG_WARNING() << "Failed to add channel: '" << ex.what() << "'";
          }
        }
      });
}

ChannelPool::~ChannelPool() {
  Stop();

  // Since channel is destroyed in ev-thread we just run this in ev altogether
  // to reduce overhead
  thread_.RunInEvLoopSync([this] {
    while (true) {
      auto* ptr = TryPop();
      if (!ptr) break;

      Drop(ptr);
    }
  });
}

ChannelPtr ChannelPool::Acquire() { return {shared_from_this(), Pop()}; }

void ChannelPool::Release(
    std::unique_ptr<impl::IAmqpChannel>&& channel) noexcept {
  UASSERT(channel.get());
  channel->ResetCallbacks();

  auto* ptr = channel.release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
  given_away_.fetch_add(-1, std::memory_order_relaxed);
  size_.fetch_add(1, std::memory_order_relaxed);
}

void ChannelPool::NotifyChannelAdopted() noexcept {
  given_away_.fetch_add(-1, std::memory_order_relaxed);
}

void ChannelPool::Stop() noexcept {
  size_monitor_.Stop();
  connection_ = nullptr;
}

impl::IAmqpChannel* ChannelPool::Pop() {
  auto* ptr = TryPop();
  if (!ptr) {
    // TODO : fix me
    throw std::runtime_error{"Failed to acquire a channel"};
  }
  given_away_.fetch_add(1, std::memory_order_relaxed);
  size_.fetch_add(-1, std::memory_order_relaxed);
  return ptr;
}

impl::IAmqpChannel* ChannelPool::TryPop() {
  impl::IAmqpChannel* ptr{nullptr};
  if (queue_.pop(ptr)) {
    return ptr;
  }

  return nullptr;
}

std::unique_ptr<impl::IAmqpChannel> ChannelPool::CreateChannel() {
  UASSERT_MSG(connection_ != nullptr,
              "An attempt to create a channel after parent destructor called");

  const auto deadline = engine::Deadline::FromDuration(kChannelCreateTimeout);
  if (reliable_) {
    return std::make_unique<impl::AmqpReliableChannel>(*connection_, deadline);
  } else {
    return std::make_unique<impl::AmqpChannel>(*connection_, deadline);
  }
}

void ChannelPool::Drop(impl::IAmqpChannel* channel) {
  std::default_delete<impl::IAmqpChannel>{}(channel);
  size_.fetch_add(-1, std::memory_order_relaxed);
}

void ChannelPool::AddChannel() {
  auto* ptr = CreateChannel().release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }

  size_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
