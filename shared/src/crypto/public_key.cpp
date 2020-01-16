#include <crypto/public_key.hpp>

#include <crypto/certificate.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <boost/algorithm/string/predicate.hpp>  // for boost::starts_with

#include <crypto/exception.hpp>
#include <crypto/hash.hpp>
#include <crypto/helpers.hpp>
#include <crypto/openssl_lock.hpp>

namespace crypto {
namespace {

int NoPasswordCb(char* /*buf*/, int /*size*/, int /*rwflag*/, void*) {
  return 0;
}

}  // namespace

PublicKey PublicKey::LoadFromString(utils::string_view key) {
  impl::OpensslLock::Init();

  if (boost::starts_with(key, "-----BEGIN CERTIFICATE-----")) {
    return LoadFromCertificate(Certificate::LoadFromString(key));
  }

  auto pubkey_bio = MakeBioString(key);

  std::shared_ptr<EVP_PKEY> pubkey(
      ::PEM_read_bio_PUBKEY(pubkey_bio.get(), nullptr, &NoPasswordCb, nullptr),
      ::EVP_PKEY_free);
  if (!pubkey) throw KeyParseError(FormatSslError("Failed to load public key"));
  return PublicKey{std::move(pubkey)};
}

PublicKey PublicKey::LoadFromCertificate(const Certificate& cert) {
  std::shared_ptr<EVP_PKEY> pubkey(::X509_get_pubkey(cert.GetNative()),
                                   ::EVP_PKEY_free);
  if (!pubkey) {
    throw KeyParseError(
        FormatSslError("Error getting public key from certificate"));
  }
  return PublicKey{std::move(pubkey)};
}

}  // namespace crypto
