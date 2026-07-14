#pragma once

#include <cstddef>

#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::statistics {

class ConnectionStatistics final {
public:
    void AccountConnectionCreated();
    void AccountConnectionClosed() noexcept;

    void AccountWrite(size_t bytes_written);
    void AccountRead(size_t bytes_read);

    void AccountMessagePublished();
    void AccountMessageConsumed();

    struct Frozen final {
        Frozen& operator+=(const Frozen& other);

        utils::statistics::Rate connections_created;
        utils::statistics::Rate connections_closed;

        utils::statistics::Rate bytes_sent;
        utils::statistics::Rate bytes_read;

        utils::statistics::Rate messages_published;
        utils::statistics::Rate messages_consumed;
    };
    Frozen Get() const;

private:
    utils::statistics::RateCounter connections_created_{};
    utils::statistics::RateCounter connections_closed_{};

    utils::statistics::RateCounter bytes_sent_{};
    utils::statistics::RateCounter bytes_read_{};

    utils::statistics::RateCounter messages_published_{};
    utils::statistics::RateCounter messages_consumed_{};
};

void DumpMetric(utils::statistics::Writer& writer, const ConnectionStatistics::Frozen& value);

}  // namespace urabbitmq::statistics

USERVER_NAMESPACE_END
