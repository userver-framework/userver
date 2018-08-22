#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <yandex/taxi/userver/logging/logger.hpp>

#include "response_base.hpp"

namespace server {
namespace request {

class RequestBase {
 public:
  RequestBase();
  virtual ~RequestBase();

  virtual ResponseBase& GetResponse() const = 0;

  virtual void WriteAccessLogs(const logging::LoggerPtr& logger_access,
                               const logging::LoggerPtr& logger_access_tskv,
                               const std::string& remote_address) const = 0;

  virtual const std::string& GetRequestPath() const = 0;

  void SetTaskCreateTime();
  void SetTaskStartTime();
  void SetResponseNotifyTime();
  void SetCompleteNotifyTime();
  void SetStartSendResponseTime();
  void SetFinishSendResponseTime();

  virtual void SetMatchedPathLength(size_t length) = 0;

 protected:
  std::chrono::steady_clock::time_point StartTime() const {
    return start_time_;
  }

 private:
  std::chrono::steady_clock::time_point start_time_;
  std::chrono::steady_clock::time_point task_create_time_;
  std::chrono::steady_clock::time_point task_start_time_;
  std::chrono::steady_clock::time_point response_notify_time_;
  std::chrono::steady_clock::time_point complete_notify_time_;
  std::chrono::steady_clock::time_point start_send_response_time_;
  std::chrono::steady_clock::time_point finish_send_response_time_;
};

}  // namespace request
}  // namespace server
