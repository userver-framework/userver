#pragma once

#include <string>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Channel final {
 public:
  Channel();
  ~Channel();

  void Publish(const Exchange& exchange, const std::string& message);

  void PublishReliable(const Exchange& exchange, const std::string& message);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}

USERVER_NAMESPACE_END