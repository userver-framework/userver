#include "connection_statistics.hpp"

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {

void ConnectionStatistics::AccountConnectionCreated() {
  ++connections_created_;
}

void ConnectionStatistics::AccountConnectionClosed() noexcept {
  ++connections_closed_;
}

void ConnectionStatistics::AccountWrite(size_t bytes_written) {
  bytes_sent_ += bytes_written;
}

void ConnectionStatistics::AccountRead(size_t bytes_read) {
  bytes_read_ += bytes_read;
}

void ConnectionStatistics::AccountMessagePublished() { ++messages_published_; }

void ConnectionStatistics::AccountMessageConsumed() { ++messages_consumed_; }

ConnectionStatistics::Frozen ConnectionStatistics::Get() const {
  Frozen result{};
  result.connections_created = connections_created_.Load();
  result.connections_closed = connections_closed_.Load();
  result.bytes_sent = bytes_sent_.Load();
  result.bytes_read = bytes_read_.Load();
  result.messages_published = messages_published_.Load();
  result.messages_consumed = messages_consumed_.Load();

  return result;
}

ConnectionStatistics::Frozen& ConnectionStatistics::Frozen::operator+=(
    const Frozen& other) {
  connections_created += other.connections_created;
  connections_closed += other.connections_closed;
  bytes_sent += other.bytes_sent;
  bytes_read += other.bytes_read;
  messages_published += other.messages_published;
  messages_consumed += other.messages_consumed;

  return *this;
}

formats::json::Value Serialize(const ConnectionStatistics::Frozen& value,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  builder["connections_created"] = value.connections_created;
  builder["connections_closed"] = value.connections_closed;
  builder["bytes_sent"] = value.bytes_sent;
  builder["bytes_read"] = value.bytes_read;
  builder["messages_published"] = value.messages_published;
  builder["messages_consumed"] = value.messages_consumed;

  return builder.ExtractValue();
}

}  // namespace urabbitmq::statistics

USERVER_NAMESPACE_END
