#include <crypto/load_key.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <boost/algorithm/string/predicate.hpp>  // for boost::starts_with

#include <crypto/exception.hpp>
#include <crypto/hash.hpp>
#include <crypto/helpers.hpp>

namespace crypto {
namespace {

int NoPasswordCb(char* /*buf*/, int /*size*/, int /*rwflag*/, void*) {
  return 0;
}

int StringViewPasswordCb(char* buf, int size, int /*rwflag*/, void* str_vptr) {
  if (!str_vptr || !buf || size < 0) return -1;

  auto* str_ptr = static_cast<const utils::string_view*>(str_vptr);
  if (str_ptr->size() > static_cast<size_t>(size)) return -1;
  std::memcpy(buf, str_ptr->data(), str_ptr->size());
  return str_ptr->size();
}

std::shared_ptr<EVP_PKEY> LoadPublicKeyFromCert(
    utils::string_view cert_string) {
  std::unique_ptr<BIO, decltype(&BIO_free_all)> certbio(
      BIO_new_mem_buf(cert_string.data(), cert_string.size()), BIO_free_all);

  std::unique_ptr<X509, decltype(&X509_free)> cert(
      PEM_read_bio_X509(certbio.get(), nullptr, &NoPasswordCb, nullptr),
      X509_free);
  if (!cert) {
    throw KeyParseError(FormatSslError("Error loading cert into memory"));
  }

  std::shared_ptr<EVP_PKEY> pubkey(X509_get_pubkey(cert.get()), EVP_PKEY_free);
  if (!pubkey) {
    throw KeyParseError(
        FormatSslError("Error getting public key from certificate"));
  }
  return pubkey;
}

}  // namespace

std::shared_ptr<EVP_PKEY> LoadPrivateKeyFromString(
    utils::string_view key, utils::string_view password) {
  std::unique_ptr<BIO, decltype(&BIO_free_all)> privkey_bio(
      BIO_new_mem_buf(key.data(), key.size()), BIO_free_all);

  std::shared_ptr<EVP_PKEY> privkey(
      PEM_read_bio_PrivateKey(privkey_bio.get(), nullptr, &StringViewPasswordCb,
                              reinterpret_cast<void*>(&password)),
      EVP_PKEY_free);
  if (!privkey) {
    throw KeyParseError(FormatSslError("Failed to load private key"));
  }
  return privkey;
}

std::shared_ptr<EVP_PKEY> LoadPublicKeyFromString(utils::string_view key) {
  if (boost::starts_with(key, "-----BEGIN CERTIFICATE-----")) {
    return LoadPublicKeyFromCert(key);
  }

  std::unique_ptr<BIO, decltype(&BIO_free_all)> pubkey_bio(
      BIO_new_mem_buf(key.data(), key.size()), BIO_free_all);

  std::shared_ptr<EVP_PKEY> pubkey(
      PEM_read_bio_PUBKEY(pubkey_bio.get(), nullptr, &NoPasswordCb, nullptr),
      EVP_PKEY_free);
  if (!pubkey) throw KeyParseError(FormatSslError("Failed to load public key"));
  return pubkey;
}

}  // namespace crypto
