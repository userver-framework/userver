#include <storages/postgres/detail/rtt.hpp>

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {
namespace {

// in MongoDB it was chosen to put 85% of the weight to rtt to last 9 measurements
constexpr double kRttEwmaAlpha = 0.2;

}  // namespace

void UpdateAverageRtt(Rtt& average_rtt, Rtt latest_rtt) noexcept {
    if (latest_rtt == kUnknownRtt) {
        return;
    }
    if (average_rtt == kUnknownRtt) {
        average_rtt = latest_rtt;
        return;
    }

    average_rtt = std::chrono::duration_cast<Rtt>(kRttEwmaAlpha * latest_rtt + (1.0 - kRttEwmaAlpha) * average_rtt);
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
