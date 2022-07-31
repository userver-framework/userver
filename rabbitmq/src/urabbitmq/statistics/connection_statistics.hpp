#pragma once

#include <cstddef>

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {

class ConnectionStatistics final {
 public:
  void AccountReconnectSuccess();
  void AccountReconnectFailure();

  void AccountChannelCreated();
  void AccountChannelClosed();

  void AccountWrite(size_t bytes_written);
  void AccountRead(size_t bytes_read);

  struct Frozen final {
    Frozen& operator+=(const Frozen& other);

    size_t reconnect_success{0};
    size_t reconnect_failure{0};

    size_t channels_created{0};
    size_t channels_closed{0};

    size_t bytes_sent{0};
    size_t bytes_read{0};
  };
  Frozen Get() const;

 private:
  utils::statistics::RelaxedCounter<size_t> reconnect_success_{0};
  utils::statistics::RelaxedCounter<size_t> reconnect_failure_{0};

  utils::statistics::RelaxedCounter<size_t> channels_created_{0};
  utils::statistics::RelaxedCounter<size_t> channels_closed_{0};

  utils::statistics::RelaxedCounter<size_t> bytes_sent_{0};
  utils::statistics::RelaxedCounter<size_t> bytes_read_{0};
};

formats::json::Value Serialize(const ConnectionStatistics::Frozen& value,
                               formats::serialize::To<formats::json::Value>);

}  // namespace urabbitmq::statistics

USERVER_NAMESPACE_END
