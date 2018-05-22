#pragma once

#include <cstddef>
#include <vector>

namespace server {
namespace net {

struct Stats {
  class AggregatedStat {
   public:
    size_t Total() const;
    size_t Max() const;
    const std::vector<size_t>& Items() const;

    void Add(size_t item);
    AggregatedStat& operator+=(const AggregatedStat&);

   private:
    size_t total_ = 0;
    size_t max_item_ = 0;
    std::vector<size_t> items_;
  };

  // per listener
  AggregatedStat active_connections;
  AggregatedStat total_opened_connections;
  AggregatedStat total_closed_connections;
  AggregatedStat listener_processed_requests;

  // per connection
  AggregatedStat active_requests;
  AggregatedStat parsing_requests;
  AggregatedStat pending_responses;
  AggregatedStat conn_processed_requests;
};

inline Stats& operator+=(Stats& lhs, const Stats& rhs) {
  lhs.active_connections += rhs.active_connections;
  lhs.total_opened_connections += rhs.total_opened_connections;
  lhs.total_closed_connections += rhs.total_closed_connections;
  lhs.listener_processed_requests += rhs.listener_processed_requests;
  lhs.active_requests += rhs.active_requests;
  lhs.parsing_requests += rhs.parsing_requests;
  lhs.pending_responses += rhs.pending_responses;
  lhs.conn_processed_requests += rhs.conn_processed_requests;
  return lhs;
}

inline Stats operator+(Stats&& lhs, const Stats& rhs) {
  return std::move(lhs += rhs);
}

inline Stats operator+(const Stats& lhs, const Stats& rhs) {
  return Stats(lhs) + rhs;
}

inline size_t Stats::AggregatedStat::Total() const { return total_; }
inline size_t Stats::AggregatedStat::Max() const { return max_item_; }
inline const std::vector<size_t>& Stats::AggregatedStat::Items() const {
  return items_;
}

inline void Stats::AggregatedStat::Add(size_t item) {
  total_ += item;
  if (item > max_item_) {
    max_item_ = item;
  }
  items_.push_back(item);
}

inline Stats::AggregatedStat& Stats::AggregatedStat::operator+=(
    const Stats::AggregatedStat& rhs) {
  for (auto item : rhs.items_) {
    Add(item);
  }
  return *this;
}

inline Stats::AggregatedStat operator+(Stats::AggregatedStat&& lhs,
                                       const Stats::AggregatedStat& rhs) {
  return std::move(lhs += rhs);
}

inline Stats::AggregatedStat operator+(const Stats::AggregatedStat& lhs,
                                       const Stats::AggregatedStat& rhs) {
  return Stats::AggregatedStat(lhs) + rhs;
}

}  // namespace net
}  // namespace server
