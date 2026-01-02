#include <userver/crypto/certificate.hpp>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <userver/crypto/exception.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/crypto/openssl.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>

#include <crypto/helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {
namespace {

int NoPasswordCb(char* /*buf*/, int /*size*/, int /*rwflag*/, void*) { return 0; }

}  // namespace

constexpr std::string_view kBeginMarker = "-----BEGIN CERTIFICATE-----";
constexpr std::string_view kEndMarker = "-----END CERTIFICATE-----";

std::optional<std::string> Certificate::GetPemString() const {
    if (!cert_) {
        return {};
    }

    auto membio = MakeBioMemoryBuffer();
    if (1 != PEM_write_bio_X509(membio.get(), GetNative())) {
        throw SerializationError(FormatSslError("Error serializing cert to PEM"));
    }

    std::string result;
    result.resize(BIO_pending(membio.get()));
    size_t readbytes = 0;
    if (1 != BIO_read_ex(membio.get(), result.data(), result.size(), &readbytes)) {
        throw SerializationError(FormatSslError("Error transferring PEM to string"));
    }
    if (readbytes != result.size()) {
        throw SerializationError("Error transferring PEM to string");
    }
    return result;
}

Certificate Certificate::LoadFromString(std::string_view certificate) {
    Openssl::Init();

    if (!utils::text::StartsWith(certificate, kBeginMarker)) {
        throw KeyParseError(FormatSslError("Not a certificate"));
    }

    auto certbio = MakeBioString(certificate);

    std::shared_ptr<X509> cert(PEM_read_bio_X509(certbio.get(), nullptr, &NoPasswordCb, nullptr), X509_free);
    if (!cert) {
        throw KeyParseError(FormatSslError("Error loading cert into memory"));
    }
    return Certificate{std::move(cert)};
}

Certificate Certificate::LoadFromStringSkippingAttributes(std::string_view certificate) {
    const auto start = certificate.find(kBeginMarker);
    if (start == std::string::npos) {
        throw KeyParseError(FormatSslError("Not a certificate"));
    }
    return LoadFromString(certificate.substr(start));
}

CertificatesChain LoadCertificatesChainFromString(std::string_view chain) {
    CertificatesChain certificates;

    std::size_t start = 0;
    while ((start = chain.find(kBeginMarker, start)) != std::string::npos) {
        auto end = chain.find(kEndMarker, start);
        UINVARIANT(end != std::string::npos, "No matching end marker found for certificate");

        end += kEndMarker.length();
        certificates.push_back(Certificate::LoadFromString(chain.substr(start, end - start)));
        start = end;  // Move past the current certificate
    }
    if (certificates.empty()) {
        throw KeyParseError(FormatSslError("There are no certificates in chain"));
    }

    return certificates;
}

std::string Certificate::GetSubject() const {
    X509* x509 = GetNative();
    if (!x509) {
        throw KeyParseError(FormatSslError("Invalid certificate"));
    }

    X509_NAME* subject_name = X509_get_subject_name(x509);
    if (!subject_name) {
        throw KeyParseError(FormatSslError("Failed to get subject name from certificate"));
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        throw KeyParseError(FormatSslError("Failed to create BIO"));
    }

    const int flags = XN_FLAG_RFC2253;
    if (X509_NAME_print_ex(bio, subject_name, 0, flags) < 0) {
        BIO_free(bio);
        throw KeyParseError(FormatSslError("Failed to print subject name"));
    }

    char* data = nullptr;
    long len = BIO_get_mem_data(bio, &data);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    std::string result(data, len);

    BIO_free(bio);
    return result;
}

}  // namespace crypto

USERVER_NAMESPACE_END
