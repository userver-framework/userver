#pragma once

#include <memory>

namespace server {
namespace handlers {

class HandlerData {
 public:
  virtual ~HandlerData() {}
};

class HandlerContext {
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

 private:
  std::unique_ptr<HandlerData> data_;
};

}  // namespace handlers
}  // namespace server
