#pragma once

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <server/handlers/http_handler_base.hpp>
#include <server/http/handler_methods.hpp>
#include <utils/statistics.hpp>
#include <utils/statistics/percentile.hpp>
#include <utils/statistics/recentperiod.hpp>

namespace server {
namespace handlers {

class ReplyCodesStatistics {
 public:
  std::unordered_map<unsigned int, size_t> GetReplyCodes() const {
    std::unordered_map<unsigned int, size_t> codes;

    for (size_t i = 1; i < reply_codes_100_.size(); i++) {
      const size_t count = reply_codes_100_[i];
      codes[i * 100] = count;
    }

    codes[401] = reply_401_;

    return codes;
  }

  void Account(unsigned int code) {
    if (code == 401) reply_401_++;
    if (code < 100) code = 100;
    int code_100 = std::min<size_t>(code / 100, reply_codes_100_.size() - 1);
    reply_codes_100_[code_100]++;
  }

 private:
  std::array<std::atomic<size_t>, 6> reply_codes_100_{};
  std::atomic<size_t> reply_401_{0};
};

class HttpHandlerBase::Statistics {
 public:
  void Account(unsigned int code, size_t ms) {
    reply_codes_.Account(code);
    timings_.GetCurrentCounter().Account(ms);
  }

  std::unordered_map<unsigned int, size_t> GetReplyCodes() const {
    return reply_codes_.GetReplyCodes();
  }

  using Percentile = utils::statistics::Percentile<2048, unsigned int, 120>;

  Percentile GetTimings() const { return timings_.GetStatsForPeriod(); }

  size_t GetInFlight() const { return in_flight_; }

  void IncrementInFlight() { in_flight_++; }

  void DecrementInFlight() { in_flight_--; }

 private:
  utils::statistics::RecentPeriod<Percentile, Percentile,
                                  utils::datetime::SteadyClock>
      timings_;
  ReplyCodesStatistics reply_codes_;
  std::atomic<size_t> in_flight_{0};
};

class HttpHandlerBase::HandlerStatistics {
 public:
  Statistics& GetStatisticByMethod(http::HttpMethod method);

  Statistics& GetTotalStatistics();

  void Account(http::HttpMethod method, unsigned int code,
               std::chrono::milliseconds ms);

  bool IsOkMethod(http::HttpMethod method) const;

 private:
  Statistics stats_;
  std::array<Statistics, http::kHandlerMethodsMax + 1> stats_by_method_;
};

class HttpHandlerBase::HttpHandlerStatisticsScope {
 public:
  HttpHandlerStatisticsScope(HttpHandlerBase::HandlerStatistics& stats,
                             http::HttpMethod method);

  void Account(unsigned int code, std::chrono::milliseconds ms);

 private:
  HttpHandlerBase::HandlerStatistics& stats_;
  const http::HttpMethod method_;
};

}  // namespace handlers
}  // namespace server
