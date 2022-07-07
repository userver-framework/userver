#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel;
class Channel;

class Cluster final {
 public:
  std::shared_ptr<AdminChannel> GetAdminChannel();
  std::shared_ptr<Channel> GetChannel();

 private:
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
