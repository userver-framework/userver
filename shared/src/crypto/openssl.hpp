#pragma once

namespace crypto::impl {

class Openssl final {
 public:
  static void Init() noexcept;

 private:
  Openssl() noexcept;
};

}  // namespace crypto::impl
