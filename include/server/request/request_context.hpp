#pragma once

#include <memory>

#include <tracing/span.hpp>

namespace server {
namespace request {

class RequestData {
 public:
  virtual ~RequestData() {}
};

class RequestContext {
 public:
  RequestContext() = default;
  RequestContext(RequestContext&&) = delete;
  RequestContext(const RequestContext&) = delete;

  template <typename Data, typename... Args>
  void EmplaceData(Args&&... args) {
    data_ = std::make_unique<Data>(std::forward<Args>(args)...);
  }

  template <typename Data>
  Data* GetData() {
    return dynamic_cast<Data*>(data_.get());
  }

  void ClearData() { data_.reset(); }

 private:
  std::unique_ptr<RequestData> data_;
};

}  // namespace request
}  // namespace server
