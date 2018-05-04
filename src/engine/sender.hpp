#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>

#include <engine/ev/thread_control.hpp>

#include "socket_listener.hpp"

namespace engine {

class Sender : public ev::ThreadControl {
 public:
  using OnCompleteFunc = std::function<void()>;
  using Result = SocketListener::Result;

  Sender(const ev::ThreadControl& thread_control, TaskProcessor& task_processor,
         int fd, OnCompleteFunc&& on_complete);
  virtual ~Sender();

  void Start();
  void Stop();
  void SendData(std::string data, std::function<void(size_t)>&& finish_cb);

  bool Stopped() const { return stopped_; }
  size_t DataQueueSize() const;
  bool HasWaitingData() const;

 private:
  struct Elem {
    Elem(std::string&& data, std::function<void(size_t)>&& finish_cb)
        : data(std::move(data)), finish_cb(std::move(finish_cb)) {}

    std::string data;
    std::function<void(size_t)> finish_cb;
  };

  const std::string& CurrentData(std::lock_guard<std::mutex>& lock);
  Result SendCurrentData(std::lock_guard<std::mutex>& lock, int fd);
  Result SendData(int fd);

  mutable std::mutex data_queue_mutex_;
  std::deque<Elem> data_queue_;
  size_t current_data_pos_;

  std::atomic<bool> stopped_;
  const OnCompleteFunc on_complete_;

  SocketListener socket_listener_;
};

}  // namespace engine
