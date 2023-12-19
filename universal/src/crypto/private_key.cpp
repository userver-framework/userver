#include <userver/crypto/private_key.hpp>

#include <openssl/evp.h>
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

std::optional<std::string> GetPemStringImpl(EVP_PKEY* key,
                                            const EVP_CIPHER* enc,
                                            std::string_view password) {
  if (enc && password.empty()) {
    throw SerializationError(
        "Attempt to export private key with an empty password");
  }
  if (!key) return {};

  auto membio = MakeBioSecureMemoryBuffer();
  if (1 != PEM_write_bio_PrivateKey(membio.get(), key, enc, nullptr, 0,
                                    &crypto::StringViewPasswordCb,
                                    reinterpret_cast<void*>(&password))) {
    throw SerializationError(FormatSslError("Error serializing key to PEM"));
  }

  std::string result;
  result.resize(BIO_pending(membio.get()));
  size_t readbytes = 0;
  if (1 !=
      BIO_read_ex(membio.get(), result.data(), result.size(), &readbytes)) {
    throw SerializationError(
        FormatSslError("Error transferring PEM to string"));
  }
  if (readbytes != result.size()) {
    throw SerializationError("Error transferring PEM to string");
  }
  return result;
}

}  // namespace

std::optional<std::string> PrivateKey::GetPemString(
    std::string_view password) const {
  return GetPemStringImpl(GetNative(), EVP_aes_128_cbc(), password);
}

std::optional<std::string> PrivateKey::GetPemStringUnencrypted() const {
  return GetPemStringImpl(GetNative(), nullptr, {});
}

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
