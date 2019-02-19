#include <crypto/algorithm.hpp>

#include <cryptopp/misc.h>

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

namespace crypto::algorithm {

bool AreStringsEqualConstTime(const std::string& str1,
                              const std::string& str2) {
  if (str1.size() != str2.size()) return false;
  return CryptoPP::VerifyBufsEqual(reinterpret_cast<const byte*>(str1.data()),
                                   reinterpret_cast<const byte*>(str2.data()),
                                   str1.size());
}

}  // namespace crypto::algorithm
