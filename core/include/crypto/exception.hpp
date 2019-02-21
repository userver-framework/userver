#pragma once

#include <utils/traceful_exception.hpp>

namespace crypto {

class CryptoException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

}  // namespace crypto
