#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <curl-ev/error_code.hpp>
#include <userver/cache/lru_map.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

class ConnectRateLimiter {
 public:
  ConnectRateLimiter();

  void SetGlobalHttpLimits(size_t limit, utils::TokenBucket::Duration rate);
  void SetGlobalHttpsLimits(size_t limit, utils::TokenBucket::Duration rate);

  void SetPerHostLimits(size_t limit, utils::TokenBucket::Duration rate);

  void Check(const char* url_str, std::error_code& ec);

 private:
  utils::TokenBucket global_http_;
  utils::TokenBucket global_https_;

  std::atomic<size_t> per_host_limit_;
  std::atomic<utils::TokenBucket::Duration> per_host_rate_;
  concurrent::Variable<cache::LruMap<std::string, utils::TokenBucket>,
                       std::mutex>
      by_host_;
};

}  // namespace curl

USERVER_NAMESPACE_END
