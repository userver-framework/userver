#pragma once

#include <cstddef>

#include <userver/utils/statistics/relaxed_counter.hpp>
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

    size_t connections_created{0};
    size_t connections_closed{0};

    size_t bytes_sent{0};
    size_t bytes_read{0};

    size_t messages_published{0};
    size_t messages_consumed{0};
  };
  Frozen Get() const;

 private:
  utils::statistics::RelaxedCounter<size_t> connections_created_{0};
  utils::statistics::RelaxedCounter<size_t> connections_closed_{0};

  utils::statistics::RelaxedCounter<size_t> bytes_sent_{0};
  utils::statistics::RelaxedCounter<size_t> bytes_read_{0};

  utils::statistics::RelaxedCounter<size_t> messages_published_{0};
  utils::statistics::RelaxedCounter<size_t> messages_consumed_{0};
};

void DumpMetric(utils::statistics::Writer& writer,
                const ConnectionStatistics::Frozen& value);

}  // namespace urabbitmq::statistics

USERVER_NAMESPACE_END
