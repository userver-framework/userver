#include <crypto/algorithm.hpp>

#include <cryptopp/misc.h>

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

namespace crypto::algorithm {

bool AreStringsEqualConstTime(std::string_view str1, std::string_view str2) {
  if (str1.size() != str2.size()) return false;
  return CryptoPP::VerifyBufsEqual(reinterpret_cast<const byte*>(str1.data()),
                                   reinterpret_cast<const byte*>(str2.data()),
                                   str1.size());
}

}  // namespace crypto::algorithm
