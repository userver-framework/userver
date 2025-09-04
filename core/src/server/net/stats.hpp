#pragma once

#include <atomic>
#include <cstddef>

#include <userver/concurrent/striped_counter.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/striped_rate_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct Http2Stats {
    utils::statistics::RateCounter streams_count;
    utils::statistics::RateCounter streams_parse_error;
    utils::statistics::RateCounter streams_close;
    utils::statistics::RateCounter reset_streams;
    utils::statistics::RateCounter goaway;
};

struct ParserStats {
    concurrent::StripedCounter parsing_request_count;
    Http2Stats http2_stats;
};

struct ParserStatsAggregation final {
    ParserStatsAggregation() = default;

    explicit ParserStatsAggregation(const ParserStats& stats)
        : parsing_request_count{stats.parsing_request_count.NonNegativeRead()},
          streams_count(stats.http2_stats.streams_count.Load()),
          streams_parse_error(stats.http2_stats.streams_parse_error.Load()),
          streams_close(stats.http2_stats.streams_close.Load()),
          reset_streams(stats.http2_stats.reset_streams.Load()),
          goaway(stats.http2_stats.goaway.Load()) {}

    ParserStatsAggregation& operator+=(const ParserStatsAggregation& other) {
        parsing_request_count += other.parsing_request_count;
        streams_count += other.streams_count;
        streams_parse_error += other.streams_parse_error;
        streams_close += other.streams_close;
        reset_streams += other.reset_streams;
        goaway += other.goaway;

        return *this;
    }

    std::size_t parsing_request_count{0};
    // HTTP/2.0
    utils::statistics::Rate streams_count;
    utils::statistics::Rate streams_parse_error;
    utils::statistics::Rate streams_close;
    utils::statistics::Rate reset_streams;
    utils::statistics::Rate goaway;
};

struct Stats {
    // per listener
    std::atomic<std::size_t> active_connections{0};
    utils::statistics::RateCounter connections_created;
    utils::statistics::RateCounter connections_closed;

    // per connection
    ParserStats parser_stats;
    concurrent::StripedCounter active_request_count;
    utils::statistics::StripedRateCounter requests_processed_count;
};

struct StatsAggregation final {
    StatsAggregation() = default;

    explicit StatsAggregation(const Stats& stats)
        : active_connections{stats.active_connections.load()},
          connections_created{stats.connections_created.Load()},
          connections_closed{stats.connections_closed.Load()},
          parser_stats{stats.parser_stats},
          active_request_count{stats.active_request_count.NonNegativeRead()},
          requests_processed_count{stats.requests_processed_count.Load()} {}

    StatsAggregation& operator+=(const StatsAggregation& other) {
        active_connections += other.active_connections;
        connections_created += other.connections_created;
        connections_closed += other.connections_closed;

        parser_stats += other.parser_stats;
        active_request_count += other.active_request_count;
        requests_processed_count += other.requests_processed_count;

        return *this;
    }

    std::size_t active_connections{0};
    utils::statistics::Rate connections_created;
    utils::statistics::Rate connections_closed;

    // per connection
    ParserStatsAggregation parser_stats;
    std::size_t active_request_count{0};
    utils::statistics::Rate requests_processed_count;
};

}  // namespace server::net

USERVER_NAMESPACE_END
