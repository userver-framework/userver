#pragma once

#include <memory>

#include <userver/utils/fast_pimpl.hpp>

/// Declaration of `librdkafka` classes.

struct rd_kafka_s;
struct rd_kafka_conf_s;
struct rd_kafka_queue_s;
struct rd_kafka_message_s;
struct rd_kafka_op_s;

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

template <class T>
using DeleterType = void (*)(T*);

/// @brief std::unique_ptr wrapper with custom deleter.
template <class T, DeleterType<T> Deleter>
class HolderBase final {
public:
    explicit HolderBase(T* data);

    HolderBase(HolderBase&&) noexcept = default;
    HolderBase& operator=(HolderBase&&) noexcept = default;

    T* GetHandle() const noexcept;
    T* operator->() const noexcept;

    explicit operator bool() const noexcept;

    void reset() noexcept;

private:
    friend class ConfHolder;
    friend class ConsumerImpl;

    T* release() noexcept;

    std::unique_ptr<T, DeleterType<T>> ptr_;
};

class ConfHolder final {
public:
    explicit ConfHolder(rd_kafka_conf_s* conf);

    ~ConfHolder();

    ConfHolder(ConfHolder&&) noexcept;
    ConfHolder& operator=(ConfHolder&&) noexcept;

    ConfHolder(const ConfHolder&);
    ConfHolder& operator=(const ConfHolder&) = delete;

    rd_kafka_conf_s* GetHandle() const noexcept;

    /// After `rd_kafka_new` previous configuration pointer must not be destroyed,
    /// because `rd_kafka_new` takes an ownership on configuration
    void ForgetUnderlyingConf() noexcept;

private:
    struct Impl;
    utils::FastPimpl<Impl, 16, 8> impl_;
};

enum class ClientType { kConsumer, kProducer };

/// @brief Kafka client wrapper (consumer or producer).
/// Holds the client and its events queue.
template <ClientType client_type>
class KafkaClientHolder final {
public:
    explicit KafkaClientHolder(ConfHolder conf);

    ~KafkaClientHolder();

    rd_kafka_s* GetHandle() const noexcept;
    rd_kafka_queue_s* GetQueue() const noexcept;

    void reset() noexcept;

private:
    struct Impl;
    utils::FastPimpl<Impl, 32, 8> impl_;
};

/// @brief Message data view.
///
/// When using `librdkafka` events API, it is only possible to obtain message
/// data as a reference to event's field. So, for message data to live, its
/// container (event) must live too.
class MessageHolder {
public:
    explicit MessageHolder(rd_kafka_op_s* event);

    ~MessageHolder();

    MessageHolder(MessageHolder&& other) noexcept;

    const rd_kafka_message_s* GetHandle() const noexcept;
    const rd_kafka_message_s* operator->() const noexcept;

private:
    struct Impl;
    utils::FastPimpl<Impl, 24, 8> impl_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
