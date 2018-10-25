#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

namespace engine {
class Sender;
}  // namespace engine

namespace server {
namespace request {

class ResponseBase {
 public:
  ResponseBase() = default;
  virtual ~ResponseBase() {}

  void SetData(std::string data);
  const std::string& GetData() const { return data_; }

  // TODO: server internals. remove from public interface
  void SetReady();
  virtual void SetSent(size_t bytes_sent);
  void SetSentTime(std::chrono::steady_clock::time_point sent_time);

  bool IsReady() const { return is_ready_; }
  bool IsSent() const { return is_sent_; }
  size_t BytesSent() const { return bytes_sent_; }
  std::chrono::steady_clock::time_point ReadyTime() const {
    return ready_time_;
  }
  std::chrono::steady_clock::time_point SentTime() const { return sent_time_; }

  virtual void SendResponse(engine::Sender& sender,
                            std::function<void(size_t)> fini_cb,
                            bool need_send) = 0;

  virtual void SetStatusServiceUnavailable() = 0;
  virtual void SetStatusOk() = 0;
  virtual void SetStatusNotFound() = 0;

 protected:
  std::string data_;
  std::chrono::steady_clock::time_point ready_time_;
  std::chrono::steady_clock::time_point sent_time_;
  size_t bytes_sent_ = 0;
  bool is_ready_ = false;
  bool is_sent_ = false;
};

}  // namespace request
}  // namespace server
