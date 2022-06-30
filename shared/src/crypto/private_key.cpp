#include <userver/crypto/private_key.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <crypto/helpers.hpp>
#include <crypto/openssl.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {
namespace {

int StringViewPasswordCb(char* buf, int size, int /*rwflag*/, void* str_vptr) {
  if (!str_vptr || !buf || size < 0) return -1;

  const auto* password = static_cast<const std::string_view*>(str_vptr);
  if (password->size() > static_cast<size_t>(size)) return -1;
  std::memcpy(buf, password->data(), password->size());
  return password->size();
}

}  // namespace

PrivateKey PrivateKey::LoadFromString(std::string_view key) {
  std::string_view empty_password{};
  return LoadFromString(key, empty_password);
}

PrivateKey PrivateKey::LoadFromString(std::string_view key,
                                      std::string_view password) {
  impl::Openssl::Init();

  auto privkey_bio = MakeBioString(key);

  std::shared_ptr<EVP_PKEY> privkey(
      ::PEM_read_bio_PrivateKey(privkey_bio.get(), nullptr,
                                &crypto::StringViewPasswordCb,
                                reinterpret_cast<void*>(&password)),
      ::EVP_PKEY_free);
  if (!privkey) {
    throw KeyParseError(FormatSslError("Failed to load private key"));
  }
  return PrivateKey{privkey};
}

}  // namespace crypto

USERVER_NAMESPACE_END
