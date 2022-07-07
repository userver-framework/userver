#pragma once

#include <memory>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Cluster;
class ConsumerBaseImpl;

class ConsumerBase {
 public:
  ConsumerBase(std::shared_ptr<Cluster> cluster, const Queue& queue);
  virtual ~ConsumerBase();

  void Start();
  void Stop();

  virtual void Process(std::string message) = 0;

 private:
  std::shared_ptr<Cluster> cluster_;

  std::unique_ptr<ConsumerBaseImpl> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
