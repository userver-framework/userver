#include <userver/crypto/ssl_ctx.hpp>

#include <fmt/core.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <crypto/helpers.hpp>
#include <userver/crypto/openssl.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

class SslCtx::Impl {
public:
    static std::unique_ptr<Impl> MakeSslCtx();

    explicit Impl(SSL_CTX* ctx)
        : ctx_{ctx}
    {}

    ~Impl() {
        if (ctx_) {
            SSL_CTX_free(ctx_);
        }
    }

    SSL_CTX* Get() noexcept { return ctx_; }

private:
    SSL_CTX* ctx_;
};

std::unique_ptr<SslCtx::Impl> SslCtx::Impl::MakeSslCtx() {
    crypto::Openssl::Init();

    SSL_CTX* ssl_ctx = SSL_CTX_new(SSLv23_method());
    if (!ssl_ctx) {
        throw CryptoException(crypto::FormatSslError("Failed create an SSL context: SSL_CTX_new"));
    }
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    if (1 != SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_VERSION)) {
        throw CryptoException(crypto::FormatSslError("Failed create an SSL context: SSL_CTX_set_min_proto_version"));
    }
#endif

    constexpr auto options =
        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
        | SSL_OP_NO_RENEGOTIATION
#endif
        ;
    SSL_CTX_set_options(ssl_ctx, options);
    SSL_CTX_set_mode(ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
    SSL_CTX_clear_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
    if (1 != SSL_CTX_set_default_verify_paths(ssl_ctx)) {
        LOG_LIMITED_WARNING() << crypto::FormatSslError("Failed create an SSL context: SSL_CTX_set_default_verify_paths"
        );
    }

    return std::make_unique<Impl>(ssl_ctx);
}

void* SslCtx::GetRawSslCtx() const noexcept { return static_cast<void*>(impl_->Get()); }

// https://www.rfc-editor.org/rfc/rfc7540#section-3.1
static constexpr unsigned char kAlpnHttp1Only[] = "\x08http/1.1";
// https://www.rfc-editor.org/rfc/rfc9112.html#section-12.4
static constexpr unsigned char kAlpnHttp2Only[] = "\x02h2";
static constexpr unsigned char kAlpnHttp2FallbackHttp1[] = "\x02h2\x08http/1.1";

static int AlpnSelectCallback(
    SSL*,
    const unsigned char** out,
    unsigned char* outlen,
    const unsigned char* in,
    unsigned int inlen,
    void* arg
) {
    auto& context = *static_cast<SslCtx*>(arg);

    if (SSL_select_next_proto(
            const_cast<unsigned char**>(out),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
            outlen,
            context.GetAlpn().data(),
            context.GetAlpn().size(),
            in,
            inlen
        ) != OPENSSL_NPN_NEGOTIATED)
    {
        LOG_ERROR() << crypto::FormatSslError("SSL_select_next_proto failed");
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }

    LOG_INFO() << "successfully negotiated ALPN";

    return SSL_TLSEXT_ERR_OK;
}

void SslCtx::SetHttpVersion(http::HttpVersion http_version) {
    switch (http_version) {
        case http::HttpVersion::k10:
        case http::HttpVersion::k11:
            alpn_ = kAlpnHttp1Only;
            LOG_INFO() << "set ALPN for HTTP/1.1 only";
            break;
        case http::HttpVersion::k2:
            alpn_ = kAlpnHttp2FallbackHttp1;
            LOG_INFO() << "set ALPN for HTTP/2 with fallback to HTTP/1.1";
            break;
        case http::HttpVersion::k2Tls:
        case http::HttpVersion::k2PriorKnowledge:
            LOG_INFO() << "set ALPN for HTTP/2 only";
            alpn_ = kAlpnHttp2Only;
            break;
        default:
            LOG_INFO() << "skip setting ALPN";
            return;
    }

    SSL_CTX_set_alpn_select_cb(impl_->Get(), AlpnSelectCallback, this);
}

std::span<const unsigned char> SslCtx::GetAlpn() const noexcept { return alpn_; }

SslCtx::SslCtx(std::unique_ptr<Impl>&& impl)
    : impl_(std::move(impl))
{}

SslCtx::SslCtx(SslCtx&& other) noexcept = default;
SslCtx& SslCtx::operator=(SslCtx&& other) noexcept = default;
SslCtx::~SslCtx() = default;

void SslCtx::AddCertAuthorities(const std::vector<crypto::Certificate>& cert_authorities) {
    UASSERT(!cert_authorities.empty());
    auto* store = SSL_CTX_get_cert_store(impl_->Get());
    UASSERT(store);
    for (const auto& ca : cert_authorities) {
        if (1 != X509_STORE_add_cert(store, ca.GetNative())) {
            throw CryptoException(crypto::FormatSslError("X509_STORE_add_cert failed"));
        }
    }
}

void SslCtx::EnableVerifyClientCertificate() {
    SSL_CTX_set_verify(impl_->Get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
}

void SslCtx::SetServerName(std::string_view server_name) {
    if (server_name.empty()) {
        return;
    }

    X509_VERIFY_PARAM* verify_param = SSL_CTX_get0_param(impl_->Get());
    if (!verify_param) {
        throw CryptoException("SSL_CTX_get0_param failed");
    }
    if (1 != X509_VERIFY_PARAM_set1_host(verify_param, server_name.data(), server_name.size())) {
        throw CryptoException(crypto::FormatSslError("X509_VERIFY_PARAM_set1_host failed"));
    }
    SSL_CTX_set_verify(impl_->Get(), SSL_VERIFY_PEER, nullptr);
}

void SslCtx::SetCertificate(const crypto::Certificate& cert) {
    if (1 != SSL_CTX_use_certificate(impl_->Get(), cert.GetNative())) {
        throw CryptoException(crypto::FormatSslError("SSL_CTX_use_certificate failed"));
    }

    LOG_INFO() << "Loaded server cert: subject=" << cert.GetSubject();
}

void SslCtx::SetCertificates(const crypto::CertificatesChain& cert_chain) {
    if (cert_chain.empty()) {
        throw CryptoException("Empty certificate chain provided");
    }

    SetCertificate(*cert_chain.begin());

    if (cert_chain.size() > 1) {
        auto it = std::next(cert_chain.begin());
        for (; it != cert_chain.end(); ++it) {
            // cast in openssl1.0 macro expansion
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            if (SSL_CTX_add_extra_chain_cert(impl_->Get(), it->GetNative()) <= 0) {
                throw CryptoException(crypto::FormatSslError("SSL_CTX_add_extra_chain_cert failed"));
            }

            LOG_INFO() << "Loaded extra chain cert: subject=" << it->GetSubject();

            // After SSL_CTX_add_extra_chain_cert we should not free the cert
            const auto ret = X509_up_ref(it->GetNative());
            UASSERT(ret == 1);
        }
    }
}

void SslCtx::SetPrivateKey(const crypto::PrivateKey& key) {
    if (1 != SSL_CTX_use_PrivateKey(impl_->Get(), key.GetNative())) {
        throw CryptoException(crypto::FormatSslError("SSL_CTX_use_PrivateKey failed"));
    }

    LOG_INFO() << "Loaded server private key";

    if (1 != SSL_CTX_check_private_key(impl_->Get())) {
        throw CryptoException(crypto::FormatSslError("Private key does not match the certificate public key"));
    }
}

SslCtx SslCtx::CreateClientTlsContext(std::string_view server_name) {
    SslCtx ssl_ctx(Impl::MakeSslCtx());
    ssl_ctx.SetServerName(server_name);
    return ssl_ctx;
}

SslCtx SslCtx::CreateClientTlsContext(
    std::string_view server_name,
    const crypto::Certificate& cert,
    const crypto::PrivateKey& key,
    const std::vector<crypto::Certificate>& extra_cert_authorities
) {
    SslCtx ssl_ctx(Impl::MakeSslCtx());

    ssl_ctx.SetServerName(server_name);

    if (!extra_cert_authorities.empty()) {
        ssl_ctx.AddCertAuthorities(extra_cert_authorities);
    }

    ssl_ctx.SetCertificate(cert);
    ssl_ctx.SetPrivateKey(key);

    return ssl_ctx;
}

SslCtx SslCtx::CreateServerTlsContext(
    const crypto::CertificatesChain& cert_chain,
    const crypto::PrivateKey& key,
    const std::vector<crypto::Certificate>& extra_cert_authorities
) {
    SslCtx ssl_ctx(Impl::MakeSslCtx());

    if (!extra_cert_authorities.empty()) {
        ssl_ctx.AddCertAuthorities(extra_cert_authorities);
        ssl_ctx.EnableVerifyClientCertificate();
        LOG_INFO() << "Client SSL cert will be verified";
    } else {
        LOG_INFO() << "Client SSL cert will not be verified";
    }

    ssl_ctx.SetCertificates(cert_chain);
    ssl_ctx.SetPrivateKey(key);

    return ssl_ctx;
}

}  // namespace crypto

USERVER_NAMESPACE_END
