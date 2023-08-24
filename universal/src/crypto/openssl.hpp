#pragma once

USERVER_NAMESPACE_BEGIN

namespace crypto::impl {

class Openssl final {
 public:
  static void Init() noexcept;

 private:
  Openssl() noexcept;
};

}  // namespace crypto::impl

USERVER_NAMESPACE_END
