#include <userver/crypto/certificate.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <boost/algorithm/string/predicate.hpp>  // for boost::starts_with

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

}  // namespace

Certificate Certificate::LoadFromString(std::string_view certificate) {
  impl::Openssl::Init();

  if (!boost::starts_with(certificate, "-----BEGIN CERTIFICATE-----")) {
    throw KeyParseError(FormatSslError("Not a certificate"));
  }

  auto certbio = MakeBioString(certificate);

  std::shared_ptr<X509> cert(
      PEM_read_bio_X509(certbio.get(), nullptr, &NoPasswordCb, nullptr),
      X509_free);
  if (!cert) {
    throw KeyParseError(FormatSslError("Error loading cert into memory"));
  }
  return Certificate{std::move(cert)};
}

}  // namespace crypto

USERVER_NAMESPACE_END
