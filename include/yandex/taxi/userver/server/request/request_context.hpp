#pragma once

#include <memory>

#include <yandex/taxi/userver/logging/log_extra.hpp>

namespace server {
namespace request {

class RequestData {
 public:
  virtual ~RequestData() {}
};

class RequestContext {
 public:
  template <typename Data, typename... Args>
  void EmplaceData(Args&&... args) {
    data_ = std::make_unique<Data>(std::forward<Args>(args)...);
  }

  template <typename Data>
  Data* GetData() {
    return dynamic_cast<Data*>(data_.get());
  }

  void ClearData() { data_.reset(); }

  logging::LogExtra& GetLogExtra() { return log_extra_; }

 private:
  std::unique_ptr<RequestData> data_;
  logging::LogExtra log_extra_;
};

}  // namespace request
}  // namespace server
