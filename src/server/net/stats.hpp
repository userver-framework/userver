#pragma once

#include <cstddef>
#include <vector>

namespace server {
namespace net {

struct Stats {
  size_t total_connections = 0;
  size_t max_listener_connections = 0;
  std::vector<size_t> listener_connections;

  size_t total_processed_requests = 0;
  size_t total_active_requests = 0;
  size_t total_parsing_requests = 0;

  std::vector<size_t> conn_active_requests;
  std::vector<size_t> conn_pending_responses;
};

inline Stats& operator+=(Stats& lhs, const Stats& rhs) {
  lhs.total_connections += rhs.total_connections;
  if (rhs.max_listener_connections > lhs.max_listener_connections) {
    lhs.max_listener_connections = rhs.max_listener_connections;
  }
  lhs.listener_connections.insert(lhs.listener_connections.end(),
                                  rhs.listener_connections.cbegin(),
                                  rhs.listener_connections.cend());

  lhs.total_processed_requests += rhs.total_processed_requests;
  lhs.total_active_requests += rhs.total_active_requests;
  lhs.total_parsing_requests += rhs.total_parsing_requests;

  lhs.conn_active_requests.insert(lhs.conn_active_requests.end(),
                                  rhs.conn_active_requests.cbegin(),
                                  rhs.conn_active_requests.cend());
  lhs.conn_pending_responses.insert(lhs.conn_pending_responses.end(),
                                    rhs.conn_pending_responses.cbegin(),
                                    rhs.conn_pending_responses.cend());

  return lhs;
}

inline Stats operator+(Stats&& lhs, const Stats& rhs) {
  return std::move(lhs += rhs);
}

inline Stats operator+(const Stats& lhs, const Stats& rhs) {
  return Stats(lhs) + rhs;
}

}  // namespace net
}  // namespace server
