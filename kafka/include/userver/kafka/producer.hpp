#pragma once

#include <userver/kafka/stats.hpp>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace cppkafka {

class Configuration;
class Producer;

}  // namespace cppkafka

namespace kafka {

class Producer {
 public:
  Producer(std::unique_ptr<cppkafka::Configuration> config,
           const std::string& component_name,
           engine::TaskProcessor& producer_task_processor,
           bool is_testsuite_mode, utils::statistics::Storage& storage);

  ~Producer();

  void Send(std::string topic_name, std::string key, std::string message,
            const std::optional<int> partition_value = std::nullopt);

  void SendAsync(std::string topic_name, std::string key, std::string message,
                 const std::optional<int> partition_value = std::nullopt);

 private:
  [[nodiscard]] engine::TaskWithResult<void> SendInternal(
      std::string topic_name, std::string key, std::string message,
      const std::optional<int> partition_value);

  void Init();

 private:
  engine::Mutex mutex_;
  std::atomic<bool> first_send_ = true;

  /// @note `stats_` must be initialized before `config_`
  Stats stats_;
  std::unique_ptr<cppkafka::Configuration> config_;
  const std::string& component_name_;
  std::unique_ptr<cppkafka::Producer> producer_;
  engine::Task poll_task_;
  engine::TaskProcessor& producer_task_processor_;

  /// for testsuite
  const bool is_testsuite_mode_;

  /// @note Subscriptions must be the last fields! Add new fields above this
  /// comment.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
