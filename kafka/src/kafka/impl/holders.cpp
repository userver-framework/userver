#include <userver/kafka/impl/holders.hpp>

#include <utility>

#include <fmt/format.h>
#include <librdkafka/rdkafka.h>

#include <userver/utils/assert.hpp>

#include <kafka/impl/error_buffer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

template <class T, DeleterType<T> Deleter>
HolderBase<T, Deleter>::HolderBase(T* data) : ptr_(data, Deleter) {}

template <class T, DeleterType<T> Deleter>
T* HolderBase<T, Deleter>::GetHandle() const noexcept {
    return ptr_.get();
}

template <class T, DeleterType<T> Deleter>
T* HolderBase<T, Deleter>::operator->() const noexcept {
    return ptr_.get();
}

template <class T, DeleterType<T> Deleter>
HolderBase<T, Deleter>::operator bool() const noexcept {
    return ptr_ != nullptr;
}

template <class T, DeleterType<T> Deleter>
void HolderBase<T, Deleter>::reset() noexcept {
    ptr_.reset();
}

template <class T, DeleterType<T> Deleter>
T* HolderBase<T, Deleter>::release() noexcept {
    return ptr_.release();
}

template class HolderBase<rd_kafka_error_t, &rd_kafka_error_destroy>;
template class HolderBase<rd_kafka_event_t, &rd_kafka_event_destroy>;
template class HolderBase<rd_kafka_queue_t, &rd_kafka_queue_destroy>;
template class HolderBase<rd_kafka_topic_partition_list_t, &rd_kafka_topic_partition_list_destroy>;
template class HolderBase<const rd_kafka_metadata_t, &rd_kafka_metadata_destroy>;
template class HolderBase<rd_kafka_topic_t, &rd_kafka_topic_destroy>;

struct ConfHolder::Impl {
    explicit Impl(rd_kafka_conf_t* conf) : conf(conf) {}

    HolderBase<rd_kafka_conf_s, &rd_kafka_conf_destroy> conf;
};

ConfHolder::ConfHolder(rd_kafka_conf_t* conf) : impl_(conf) {}

ConfHolder::ConfHolder(const ConfHolder& other) : impl_(rd_kafka_conf_dup(other.GetHandle())) {}

ConfHolder::~ConfHolder() = default;

ConfHolder::ConfHolder(ConfHolder&&) noexcept = default;
ConfHolder& ConfHolder::operator=(ConfHolder&&) noexcept = default;

rd_kafka_conf_t* ConfHolder::GetHandle() const noexcept { return impl_->conf.GetHandle(); }

void ConfHolder::ForgetUnderlyingConf() noexcept { [[maybe_unused]] auto _ = impl_->conf.ptr_.release(); }

template <ClientType client_type>
struct KafkaClientHolder<client_type>::Impl {
    explicit Impl(ConfHolder conf)
        : handle([conf = std::move(conf)]() mutable {
              ErrorBuffer err_buf;

              const auto rd_kafka_client_type =
                  client_type == ClientType::kConsumer ? RD_KAFKA_CONSUMER : RD_KAFKA_PRODUCER;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
              HolderBase<rd_kafka_t, &rd_kafka_destroy> holder{
                  rd_kafka_new(rd_kafka_client_type, conf.GetHandle(), err_buf.data(), err_buf.size())};
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
              if (!holder) {
                  /// @note `librdkafka` takes ownership on conf iff
                  /// `rd_kafka_new` succeeds

                  const auto type_str = [] {
                      switch (client_type) {
                          case ClientType::kConsumer:
                              return "consumer";
                          case ClientType::kProducer:
                              return "producer";
                      }
                      UINVARIANT(false, "Unexpected rd_kafka_type value");
                  }();

                  PrintErrorAndThrow(fmt::format("create {}", type_str), err_buf);
              }

              conf.ForgetUnderlyingConf();

              if (client_type == ClientType::kConsumer) {
                  /// Redirects main queue to consumer's queue.
                  rd_kafka_poll_set_consumer(holder.GetHandle());
              }

              return holder;
          }()),
          queue([this] {
              switch (client_type) {
                  case ClientType::kConsumer:
                      return rd_kafka_queue_get_consumer(handle.GetHandle());
                  case ClientType::kProducer:
                      return rd_kafka_queue_get_main(handle.GetHandle());
              }
              UINVARIANT(false, "Unexpected rd_kafka_type value");
          }()) {
    }

    HolderBase<rd_kafka_t, &rd_kafka_destroy> handle;
    HolderBase<rd_kafka_queue_t, &rd_kafka_queue_destroy> queue;
};

template <ClientType client_type>
KafkaClientHolder<client_type>::KafkaClientHolder(ConfHolder conf) : impl_(std::move(conf)) {}

template <ClientType client_type>
KafkaClientHolder<client_type>::~KafkaClientHolder() = default;

template <ClientType client_type>
rd_kafka_t* KafkaClientHolder<client_type>::GetHandle() const noexcept {
    return impl_->handle.GetHandle();
}

template <ClientType client_type>
rd_kafka_queue_t* KafkaClientHolder<client_type>::GetQueue() const noexcept {
    return impl_->queue.GetHandle();
}

template <ClientType client_type>
void KafkaClientHolder<client_type>::reset() noexcept {
    impl_->queue.reset();
    impl_->handle.reset();
}

template class KafkaClientHolder<ClientType::kConsumer>;
template class KafkaClientHolder<ClientType::kProducer>;

struct MessageHolder::Impl {
    explicit Impl(rd_kafka_event_t* event) : holder(event), message(rd_kafka_event_message_next(holder.GetHandle())) {}

    Impl(Impl&& other) noexcept : holder(std::move(other.holder)), message(std::exchange(other.message, nullptr)) {}

    HolderBase<rd_kafka_event_t, &rd_kafka_event_destroy> holder;
    const rd_kafka_message_t* message;
};

MessageHolder::MessageHolder(rd_kafka_event_t* event) : impl_(event) {}

MessageHolder::MessageHolder(MessageHolder&& other) noexcept : impl_(std::move(other.impl_)) {}

MessageHolder::~MessageHolder() = default;

const rd_kafka_message_s* MessageHolder::GetHandle() const noexcept { return impl_->message; }

const rd_kafka_message_s* MessageHolder::operator->() const noexcept { return impl_->message; }

}  // namespace kafka::impl

USERVER_NAMESPACE_END
