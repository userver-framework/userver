#include "connection_statistics.hpp"

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {

void ConnectionStatistics::AccountReconnectSuccess() { ++reconnect_success_; }

void ConnectionStatistics::AccountReconnectFailure() { ++reconnect_failure_; }

void ConnectionStatistics::AccountChannelCreated() { ++channels_created_; }

void ConnectionStatistics::AccountChannelClosed() { ++channels_closed_; }

void ConnectionStatistics::AccountWrite(size_t bytes_written) {
  bytes_sent_ += bytes_written;
}

void ConnectionStatistics::AccountRead(size_t bytes_read) {
  bytes_read_ += bytes_read;
}

ConnectionStatistics::Frozen ConnectionStatistics::Get() const {
  Frozen result{};
  result.reconnect_success = reconnect_success_.Load();
  result.reconnect_failure = reconnect_failure_.Load();
  result.channels_created = channels_created_.Load();
  result.channels_closed = channels_closed_.Load();
  result.bytes_sent = bytes_sent_.Load();
  result.bytes_read = bytes_read_.Load();

  return result;
}

ConnectionStatistics::Frozen& ConnectionStatistics::Frozen::operator+=(
    const Frozen& other) {
  reconnect_success += other.reconnect_success;
  reconnect_failure += other.reconnect_failure;
  channels_created += other.channels_created;
  channels_closed += other.channels_closed;
  bytes_sent += other.bytes_sent;
  bytes_read += other.bytes_read;

  return *this;
}

formats::json::Value Serialize(const ConnectionStatistics::Frozen& value,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  builder["reconnect_success"] = value.reconnect_success;
  builder["reconnect_failure"] = value.reconnect_failure;
  builder["channels_created"] = value.channels_created;
  builder["channels_closed"] = value.channels_closed;
  builder["bytes_sent"] = value.bytes_sent;
  builder["bytes_read"] = value.bytes_read;

  return builder.ExtractValue();
}

}  // namespace urabbitmq::statistics

USERVER_NAMESPACE_END
