#pragma once

#include <string>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HeaderMap final {
 public:
  HeaderMap();
  ~HeaderMap();

  void Insert(std::string key, std::string value);

  bool Find(const std::string& key) const noexcept;

 private:
  class Impl;
  utils::FastPimpl<Impl, 72, 8> impl_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
