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

 private:
  friend class ConfHolder;

  std::unique_ptr<T, DeleterType<T>> ptr_;
};

using QueueHolder = HolderBase<rd_kafka_queue_t, &rd_kafka_queue_destroy>;
using MessageHolder = HolderBase<rd_kafka_message_t, &rd_kafka_message_destroy>;
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

class KafkaHolder final {
 public:
  KafkaHolder(ConfHolder conf, rd_kafka_type_t type);

  rd_kafka_t* GetHandle() const noexcept;

 private:
  HolderBase<rd_kafka_t, &rd_kafka_destroy> handle_;
};

using ConsumerHolder = KafkaHolder;
using ProducerHolder = KafkaHolder;

}  // namespace kafka::impl

USERVER_NAMESPACE_END
