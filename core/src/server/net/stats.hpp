#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct ParserStats {
  ParserStats(const ParserStats& other)
      : parsing_request_count(other.parsing_request_count.load()) {}

  ParserStats() = default;

  std::atomic<size_t> parsing_request_count{0};
};

inline ParserStats& operator+=(ParserStats& lhs, const ParserStats& rhs) {
  lhs.parsing_request_count += rhs.parsing_request_count;
  return lhs;
}

struct Stats {
  Stats(const Stats& other)
      : active_connections(other.active_connections.load()),
        connections_created(other.connections_created.load()),
        connections_closed(other.connections_closed.load()),
        parser_stats(other.parser_stats),
        active_request_count(other.active_request_count.load()),
        requests_processed_count(other.requests_processed_count.load()) {}

  Stats() = default;

  // per listener
  std::atomic<size_t> active_connections{0};
  std::atomic<size_t> connections_created{0};
  std::atomic<size_t> connections_closed{0};

  // per connection
  ParserStats parser_stats;
  std::atomic<size_t> active_request_count{0};
  std::atomic<size_t> requests_processed_count{0};
};

inline Stats& operator+=(Stats& lhs, const Stats& rhs) {
  lhs.active_connections += rhs.active_connections;
  lhs.connections_created += rhs.connections_created;
  lhs.connections_closed += rhs.connections_closed;

  lhs.parser_stats += rhs.parser_stats;
  lhs.active_request_count += rhs.active_request_count;
  lhs.requests_processed_count += rhs.requests_processed_count;
  return lhs;
}

inline Stats operator+(Stats&& lhs, const Stats& rhs) { return lhs += rhs; }

}  // namespace server::net

USERVER_NAMESPACE_END
