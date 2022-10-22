#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace engine::io {
class Socket;
}  // namespace engine::io

namespace server::request {

class ResponseDataAccounter final {
 public:
  void StartRequest(size_t size,
                    std::chrono::steady_clock::time_point create_time);

  void StopRequest(size_t size,
                   std::chrono::steady_clock::time_point create_time);

  size_t GetCurrentLevel() const { return current_; }

  size_t GetMaxLevel() const { return max_; }

  void SetMaxLevel(size_t size) { max_ = size; }

  std::chrono::milliseconds GetAvgRequestTime() const;

 private:
  std::atomic<size_t> current_{0};
  std::atomic<size_t> max_{std::numeric_limits<size_t>::max()};
  std::atomic<size_t> count_{0};
  std::atomic<size_t> time_sum_{0};
};

/// @brief Base class for all the server responses.
class ResponseBase {
 public:
  explicit ResponseBase(ResponseDataAccounter& data_accounter);
  ResponseBase(const ResponseBase&) = delete;
  ResponseBase(ResponseBase&&) = delete;
  virtual ~ResponseBase() noexcept;

  void SetData(std::string data);
  const std::string& GetData() const { return data_; }

  virtual bool IsBodyStreamed() const = 0;
  virtual bool WaitForHeadersEnd() = 0;
  virtual void SetHeadersEnd() = 0;

  /// @cond
  // TODO: server internals. remove from public interface
  void SetReady();
  void SetReady(std::chrono::steady_clock::time_point now);
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
  /// @endcond

 protected:
  void SetSent(size_t bytes_sent);
  void SetSentTime(std::chrono::steady_clock::time_point sent_time);

  class Guard final {
   public:
    Guard(ResponseDataAccounter& accounter,
          std::chrono::steady_clock::time_point create_time, size_t size)
        : accounter_(accounter), create_time_(create_time), size_(size) {
      accounter_.StartRequest(size_, create_time_);
    }

    ~Guard() { accounter_.StopRequest(size_, create_time_); }

   private:
    ResponseDataAccounter& accounter_;
    std::chrono::steady_clock::time_point create_time_;
    size_t size_;
  };

 private:
  ResponseDataAccounter& accounter_;
  std::optional<Guard> guard_;
  std::string data_;
  std::chrono::steady_clock::time_point create_time_;
  std::chrono::steady_clock::time_point ready_time_;
  std::chrono::steady_clock::time_point sent_time_;
  size_t bytes_sent_ = 0;
  bool is_ready_ = false;
  bool is_sent_ = false;
};

}  // namespace server::request

USERVER_NAMESPACE_END
