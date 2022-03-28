#include <userver/crypto/public_key.hpp>

#include <userver/crypto/certificate.hpp>

#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <boost/algorithm/string/predicate.hpp>  // for boost::starts_with
#include <boost/numeric/conversion/cast.hpp>

#include <crypto/helpers.hpp>
#include <crypto/openssl.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {
namespace {

int NoPasswordCb(char* /*buf*/, int /*size*/, int /*rwflag*/, void*) {
  return 0;
}

using Bignum = std::unique_ptr<BIGNUM, decltype(&::BN_clear_free)>;

Bignum LoadBignumFromBigEnd(const std::string_view raw) {
  int size = 0;
  try {
    size = boost::numeric_cast<int>(raw.size());
  } catch (const boost::bad_numeric_cast& ex) {
    throw KeyParseError{ex.what()};
  }

  auto* num = ::BN_bin2bn(reinterpret_cast<const unsigned char*>(raw.data()),
                          size, nullptr);
  if (num == nullptr) {
    throw KeyParseError{FormatSslError("Cannot parse BIGNUM: BN_bin2bn")};
  }
  return {num, &BN_clear_free};
}

std::unique_ptr<RSA, decltype(&::RSA_free)> LoadRsa([[maybe_unused]] Bignum n,
                                                    [[maybe_unused]] Bignum e) {
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
  std::unique_ptr<RSA, decltype(&::RSA_free)> rsa{::RSA_new(), ::RSA_free};

  if (RSA_set0_key(rsa.get(), n.get(), e.get(), nullptr) != 1) {
    throw KeyParseError{FormatSslError("Cannot set RSA public key")};
  }
  [[maybe_unused]] auto* n_unused = n.release();
  [[maybe_unused]] auto* e_unused = e.release();

  return rsa;
#else
  throw KeyParseError{"Unimplemented LoadRsa"};
#endif
}

}  // namespace

PublicKey PublicKey::LoadFromString(std::string_view key) {
  impl::Openssl::Init();

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

PublicKey PublicKey::LoadRSAFromComponents(ModulusView modulus,
                                           ExponentView exponent) {
  auto n = LoadBignumFromBigEnd(modulus.GetUnderlying());
  auto e = LoadBignumFromBigEnd(exponent.GetUnderlying());
  auto rsa = LoadRsa(std::move(n), std::move(e));

  std::shared_ptr<NativeType> pubkey{EVP_PKEY_new(), ::EVP_PKEY_free};
  if (pubkey == nullptr) {
    throw KeyParseError{FormatSslError("Cannot create EVP_PKEY")};
  }

  if (!EVP_PKEY_set1_RSA(pubkey.get(), rsa.get())) {
    throw KeyParseError{FormatSslError("Cannot set RSA key to EVP_PKEY")};
  }

  return PublicKey{std::move(pubkey)};
}

}  // namespace crypto

USERVER_NAMESPACE_END
