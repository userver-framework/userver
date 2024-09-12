#pragma once

#include <memory>

#include <librdkafka/rdkafka.h>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

template <class T>
using DeleterType = void (*)(T*);

template <class T, DeleterType<T> Deleter>
class HolderBase final {
 public:
  explicit HolderBase(T* data) : ptr_(data, Deleter) {}

  HolderBase(HolderBase&&) noexcept = default;
  HolderBase& operator=(HolderBase&&) noexcept = default;

  T* GetHandle() const noexcept { return ptr_.get(); }
  T* operator->() const noexcept { return ptr_.get(); }

  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  void reset() { ptr_.reset(); };

 private:
  friend class ConfHolder;

  std::unique_ptr<T, DeleterType<T>> ptr_;
};

using ErrorHolder = HolderBase<rd_kafka_error_t, &rd_kafka_error_destroy>;
using EventHolder = HolderBase<rd_kafka_event_t, &rd_kafka_event_destroy>;
using QueueHolder = HolderBase<rd_kafka_queue_t, &rd_kafka_queue_destroy>;
using TopicPartitionsListHolder =
    HolderBase<rd_kafka_topic_partition_list_t,
               &rd_kafka_topic_partition_list_destroy>;

class ConfHolder final {
 public:
  explicit ConfHolder(rd_kafka_conf_t* conf);

  ~ConfHolder() = default;

  ConfHolder(ConfHolder&&) noexcept = default;
  ConfHolder& operator=(ConfHolder&&) noexcept = default;

  ConfHolder(const ConfHolder&);
  ConfHolder& operator=(const ConfHolder&) = delete;

  rd_kafka_conf_t* GetHandle() const noexcept;

  /// After `rd_kafka_new` previous configuration pointer must not be destroyed,
  /// because `rd_kafka_new` takes an ownership on configuration
  void ForgetUnderlyingConf();

 private:
  HolderBase<rd_kafka_conf_t, &rd_kafka_conf_destroy> conf_;
};

template <rd_kafka_type_t client_type>
class KafkaClientHolder final {
 public:
  explicit KafkaClientHolder(ConfHolder conf);

  rd_kafka_t* GetHandle() const noexcept;
  rd_kafka_queue_t* GetQueue() const noexcept;

  void reset();

 private:
  HolderBase<rd_kafka_t, &rd_kafka_destroy> handle_;
  QueueHolder queue_;
};

using ConsumerHolder = KafkaClientHolder<RD_KAFKA_CONSUMER>;
using ProducerHolder = KafkaClientHolder<RD_KAFKA_PRODUCER>;

/// When using `librdkafka` events API, it is only possible to obtain message
/// data as a reference to event's field. So, for message data to live, its
/// container (event) must live too.
class MessageHolder {
 public:
  explicit MessageHolder(EventHolder&& event);

  MessageHolder(MessageHolder&& other) noexcept;

  const rd_kafka_message_t* GetHandle() const noexcept { return message_; }
  const rd_kafka_message_t* operator->() const noexcept { return message_; }

 private:
  EventHolder event_;
  const rd_kafka_message_t* message_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
