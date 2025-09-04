#pragma once

/// @file userver/crypto/ssl_ctx.hpp
/// @brief @copybrief crypto::SslCtx

#include <memory>
#include <string_view>
#include <vector>

#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

/// @ingroup userver_universal userver_containers
///
/// SSL context
class SslCtx {
public:
    static SslCtx CreateServerTlsContext(
        const crypto::CertificatesChain& cert_chain,
        const crypto::PrivateKey& key,
        const std::vector<crypto::Certificate>& extra_cert_authorities = {}
    );

    static SslCtx CreateClientTlsContext(std::string_view server_name);

    static SslCtx CreateClientTlsContext(
        std::string_view server_name,
        const crypto::Certificate& cert,
        const crypto::PrivateKey& key,
        const std::vector<crypto::Certificate>& extra_cert_authorities = {}
    );

    SslCtx(SslCtx&&) noexcept;
    SslCtx& operator=(SslCtx&&) noexcept;
    ~SslCtx();

    SslCtx(const SslCtx&) = delete;
    SslCtx& operator=(const SslCtx&) = delete;

    void* GetRawSslCtx() const noexcept;

private:
    void AddCertAuthorities(const std::vector<Certificate>& cert_authorities);
    void EnableVerifyClientCertificate();
    void SetServerName(std::string_view server_name);
    void SetCertificate(const crypto::Certificate& cert);
    void SetCertificates(const crypto::CertificatesChain& cert_chain);
    void SetPrivateKey(const crypto::PrivateKey& key);

    class Impl;
    std::unique_ptr<Impl> impl_{};

    explicit SslCtx(std::unique_ptr<Impl>&& impl);
};

}  // namespace crypto

USERVER_NAMESPACE_END
