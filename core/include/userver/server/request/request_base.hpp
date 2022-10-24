#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

// TODO: use fwd declarations
#include <userver/logging/logger.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestBase {
 public:
  RequestBase();
  virtual ~RequestBase();

  virtual bool IsFinal() const = 0;

  virtual ResponseBase& GetResponse() const = 0;

  virtual void WriteAccessLogs(const logging::LoggerPtr& logger_access,
                               const logging::LoggerPtr& logger_access_tskv,
                               const std::string& remote_address) const = 0;

  virtual const std::string& GetRequestPath() const = 0;

  void SetTaskCreateTime();
  void SetTaskStartTime();
  void SetResponseNotifyTime();
  void SetResponseNotifyTime(std::chrono::steady_clock::time_point now);
  void SetStartSendResponseTime();
  void SetFinishSendResponseTime();

  virtual void SetMatchedPathLength(size_t length) = 0;

  std::chrono::steady_clock::time_point StartTime() const {
    return start_time_;
  }

  virtual void MarkAsInternalServerError() const = 0;

  virtual void AccountResponseTime() = 0;

 protected:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::steady_clock::time_point start_time_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::steady_clock::time_point task_create_time_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::steady_clock::time_point task_start_time_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::steady_clock::time_point response_notify_time_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::steady_clock::time_point start_send_response_time_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::steady_clock::time_point finish_send_response_time_;
};

}  // namespace server::request

USERVER_NAMESPACE_END
