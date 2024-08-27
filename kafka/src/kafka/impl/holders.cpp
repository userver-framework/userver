#include <kafka/impl/holders.hpp>

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

KafkaHolder::KafkaHolder(ConfHolder conf, rd_kafka_type_t type)
    : handle_([conf = std::move(conf), type]() mutable {
        ErrorBuffer err_buf;

        HolderBase<rd_kafka_t, &rd_kafka_destroy> holder{rd_kafka_new(
            type, conf.GetHandle(), err_buf.data(), err_buf.size())};
        if (!holder) {
          /// @note `librdkafka` takes ownership on conf iff
          /// `rd_kafka_new` succeeds

          const auto type_str = [type] {
            switch (type) {
              case RD_KAFKA_CONSUMER:
                return "consumer";
              case RD_KAFKA_PRODUCER:
                return "producer";
            }
            UINVARIANT(false, "Unexpected rd_kafka_type");
          }();

          PrintErrorAndThrow(fmt::format("create {}", type_str), err_buf);
        }

        conf.ForgetUnderlyingConf();

        return holder;
      }()) {}

rd_kafka_t* KafkaHolder::GetHandle() const noexcept {
  return handle_.GetHandle();
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
