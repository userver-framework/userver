#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>

namespace engine {
namespace io {
class Socket;
}  // namespace io
}  // namespace engine

namespace server {
namespace request {

class ResponseDataAccounter final {
 public:
  void Get(size_t size) { current_ += size; }
  void Put(size_t size) { current_ -= size; }

  size_t GetCurrentLevel() const { return current_; }

  size_t GetMaxLevel() const { return max_; }

  void SetMaxLevel(size_t size) { max_ = size; }

 private:
  std::atomic<size_t> current_{0};
  std::atomic<size_t> max_{std::numeric_limits<size_t>::max()};
};

class ResponseBase {
 public:
  explicit ResponseBase(ResponseDataAccounter& data_accounter);
  virtual ~ResponseBase() noexcept;

  void SetData(std::string data);
  const std::string& GetData() const { return data_; }

  // TODO: server internals. remove from public interface
  void SetReady();
  virtual void SetSendFailed(
      std::chrono::steady_clock::time_point failure_time);
  bool IsLimitReached() const;

  bool IsReady() const { return is_ready_; }
  bool IsSent() const { return is_sent_; }
  size_t BytesSent() const { return bytes_sent_; }
  std::chrono::steady_clock::time_point ReadyTime() const {
    return ready_time_;
  }
  std::chrono::steady_clock::time_point SentTime() const { return sent_time_; }

  virtual void SendResponse(engine::io::Socket& socket) = 0;

  virtual void SetStatusServiceUnavailable() = 0;
  virtual void SetStatusOk() = 0;
  virtual void SetStatusNotFound() = 0;

 protected:
  void SetSent(size_t bytes_sent);
  void SetSentTime(std::chrono::steady_clock::time_point sent_time);

  ResponseDataAccounter& data_accounter_;
  std::string data_;
  std::chrono::steady_clock::time_point ready_time_;
  std::chrono::steady_clock::time_point sent_time_;
  size_t bytes_sent_ = 0;
  bool is_ready_ = false;
  bool is_sent_ = false;
};

}  // namespace request
}  // namespace server
