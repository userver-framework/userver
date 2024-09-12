#include <kafka/impl/holders.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include <kafka/impl/error_buffer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

ConfHolder::ConfHolder(rd_kafka_conf_t* conf) : conf_(conf) {}

ConfHolder::ConfHolder(const ConfHolder& other)
    : conf_(rd_kafka_conf_dup(other.conf_.GetHandle())) {}

rd_kafka_conf_t* ConfHolder::GetHandle() const noexcept {
  return conf_.GetHandle();
}

void ConfHolder::ForgetUnderlyingConf() {
  [[maybe_unused]] auto _ = conf_.ptr_.release();
}

template <rd_kafka_type_t client_type>
KafkaClientHolder<client_type>::KafkaClientHolder(ConfHolder conf)
    : handle_([conf = std::move(conf)]() mutable {
        ErrorBuffer err_buf;

        HolderBase<rd_kafka_t, &rd_kafka_destroy> holder{rd_kafka_new(
            client_type, conf.GetHandle(), err_buf.data(), err_buf.size())};
        if (!holder) {
          /// @note `librdkafka` takes ownership on conf iff
          /// `rd_kafka_new` succeeds

          const auto type_str = [] {
            switch (client_type) {
              case RD_KAFKA_CONSUMER:
                return "consumer";
              case RD_KAFKA_PRODUCER:
                return "producer";
            }
            UINVARIANT(false, "Unexpected rd_kafka_type value");
          }();

          PrintErrorAndThrow(fmt::format("create {}", type_str), err_buf);
        }

        conf.ForgetUnderlyingConf();

        if (client_type == RD_KAFKA_CONSUMER) {
          /// Redirects main queue to consumer's queue.
          rd_kafka_poll_set_consumer(holder.GetHandle());
        }

        return holder;
      }()),
      queue_([this] {
        switch (client_type) {
          case RD_KAFKA_CONSUMER:
            return rd_kafka_queue_get_consumer(GetHandle());
          case RD_KAFKA_PRODUCER:
            return rd_kafka_queue_get_main(GetHandle());
        }
        UINVARIANT(false, "Unexpected rd_kafka_type value");
      }()) {}

template <rd_kafka_type_t client_type>
rd_kafka_t* KafkaClientHolder<client_type>::GetHandle() const noexcept {
  return handle_.GetHandle();
}

template <rd_kafka_type_t client_type>
rd_kafka_queue_t* KafkaClientHolder<client_type>::GetQueue() const noexcept {
  return queue_.GetHandle();
}

template <rd_kafka_type_t client_type>
void KafkaClientHolder<client_type>::reset() {
  queue_.reset();
  handle_.reset();
}

template class KafkaClientHolder<RD_KAFKA_CONSUMER>;
template class KafkaClientHolder<RD_KAFKA_PRODUCER>;

MessageHolder::MessageHolder(EventHolder&& event)
    : event_(std::move(event)),
      message_(rd_kafka_event_message_next(event_.GetHandle())) {}

MessageHolder::MessageHolder(MessageHolder&& other) noexcept
    : event_(std::move(other.event_)),
      message_(std::exchange(other.message_, nullptr)) {}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
