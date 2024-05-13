#pragma once

#include <functional>

#include <userver/kafka/message.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class Consumer;

}  // namespace impl

// clang-format off

/// @ingroup userver_clients
///
/// @brief RAII class that used as interface for Apache Kafka Consumer interaction
/// and proper lifetime management.
///
/// Its main purpose is to stop the message polling in user
/// component (that holds `ConsumerScope`) destructor, because `ConsumerScope::Callback`
/// often captures `this` pointer on user component.
///
/// Common usage:
///
/// @snippet kafka/functional_tests/integrational_tests/kafka_service.cpp  Kafka service sample - consumer usage
///
/// ## Important implementation details
///
/// `ConsumerScope` holds reference to `impl::Consumer` that actually
/// represents the Apache Kafka Balanced Consumer.
///
/// It exposes the API for asynchronous message batches processing that is
/// polled from the subscribed topics partitions.
///
/// Consumer periodically polls the message batches from the
/// subscribed topics partitions and invokes processing callback on each batch.
///
/// Also, Consumer maintains the per topic statistics including the broker
/// connection errors.
///
/// @note Each `ConsumerScope` instance is not thread-safe. To speed up the topic
/// messages processing, create more consumers with the same `group_id`.
///
/// @see https://docs.confluent.io/platform/current/clients/consumer.html for
/// basic consumer concepts
/// @see
/// https://docs.confluent.io/platform/current/clients/librdkafka/html/md_INTRODUCTION.html#autotoc_md62
/// for understanding of balanced consumer groups
/// @warning `ConsumerScope::Start` and `ConsumerScope::Stop` maybe called multiple
/// times, but only in "start-stop" order and **NOT** concurrently
///
/// @note Must be placed as one of the last fields in the consumer component.
/// Make sure to add a comment before the field:
///
/// @code
/// // Subscription must be the last field! Add new fields above this comment.
/// @endcode

// clang-format on

class ConsumerScope final {
 public:
  /// @brief Callback that invokes on each polled message batch.
  /// @warning If callback throws, it called over and over again with the batch
  /// with the same messages, until successfull invokation.
  /// Though, user should consider idempotent message processing mechanism
  using Callback = std::function<void(MessageBatchView)>;

  /// @brief Stops the consumer (if not yet stopped).
  ~ConsumerScope();

  ConsumerScope(ConsumerScope&&) noexcept = delete;
  ConsumerScope& operator=(ConsumerScope&&) noexcept = delete;

  /// @brief Subscribes for configured topics and starts the consumer polling
  /// process.
  /// @note If `callback` throws an exception, entire message batch (also
  /// with successfully processed messages) come again, until callback succeedes
  void Start(Callback callback);

  /// @brief Revokes all topic partition consumer was subscribed on. Also closes
  /// the consumer, leaving the consumer balanced group.
  ///
  /// Called in the destructor of ConsumerScope automatically.
  ///
  /// Can be called in the beginning of your destructor if some other
  /// actions in that destructor prevent the callback from functioning
  /// correctly.
  ///
  /// After `Stop` call, subscribed topics parititions are destributed
  /// between other consumers with the same `group_id`.
  ///
  /// @warning Blocks until all `kafka::Message` destroyed.
  void Stop() noexcept;

  /// @brief Schedules the current assignment offsets committment task.
  /// Intended to be called after each message batch processing cycle.
  ///
  /// @warning Commit does not ensure that messages do not come again --
  /// they do not come again also without the commit within the same process.
  /// Commit, indeed, restricts other consumers in consumers group from reading
  /// messages already processed (committed) by the current consumer if current
  /// has stopped and leaved the group
  ///
  /// @note Instead of calling `AsyncCommit` manually, consider setting
  /// `enable_auto_commit: true` in the static config. But read Kafka
  /// documentation carefully before to understand what auto committment
  /// mechanism actually mean
  void AsyncCommit();

 private:
  friend class impl::Consumer;

  explicit ConsumerScope(impl::Consumer& consumer) noexcept;

  impl::Consumer& consumer_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
