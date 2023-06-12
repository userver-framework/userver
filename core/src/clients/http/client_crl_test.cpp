#include <userver/clients/http/client.hpp>

#include <array>

#include <fmt/format.h>

#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>

#include <userver/internal/net/net_listener.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

// Tests in this file make sure that certs do not check CRLs by default.
//
// Fails on MacOS with Segmentation fault while calling
// Request::RequestImpl::on_certificate_request, probably because CURL library
// was misconfigured and uses wrong version of OpenSSL.

USERVER_NAMESPACE_BEGIN

namespace {

constexpr int kCrlDistributionPort = 1308;

// clang-format off

/*

# CA
openssl req -newkey rsa:4096 -nodes -days 7300 -x509 -sha256 -keyout ca.key.pem -out ca.cert.pem -subj '/C=AU/ST=Some/L=Some Other/CN=RootCA' -addext crlDistributionPoints=URI:http://[::1]:1308/crl.pem

openssl x509 -in ca.cert.pem -text  # Should show 'X509v3 CRL Distribution Points'
openssl verify -verbose -x509_strict -CAfile ca.cert.pem ca.cert.pem  # Should output OK

# Server
openssl req -newkey rsa:4096 -nodes -days 7200 -keyout revoked_server.pem -out revoked_server.csr.pem -subj '/C=AU/ST=Some/L=Some Other/CN=::1'

openssl x509 -req -days 7200 -set_serial 01 -in revoked_server.csr.pem -out revoked_server.cert.pem -CA ca.cert.pem -CAkey ca.key.pem

openssl verify -CAfile ca.cert.pem revoked_server.cert.pem  # Should output OK

# Client
openssl req -newkey rsa:4096 -nodes -days 7200 -keyout revoked_client.pem -out revoked_client.csr.pem -subj '/C=AU/ST=Some/L=Some Other/CN=client'

openssl x509 -req -days 7200 -set_serial 02 -in revoked_client.csr.pem -out revoked_client.cert.pem -CA ca.cert.pem -CAkey ca.key.pem

openssl verify -CAfile ca.cert.pem revoked_client.cert.pem  # Should output OK

# Revoking
mkdir crl && cd crl && touch index.txt.attr && touch index.txt && echo 00 > pulp_crl_number

cat <<EOF > crl_openssl.conf
[ ca ]
default_ca = CA_default    # The default ca section

[ CA_default ]
database = crl/index.txt
crlnumber = crl/pulp_crl_number


default_days = 36500
default_crl_days = 3000   # how long before next CRL
default_md = default      # use public key default MD
preserve = no             # keep passed DN ordering

[ crl_ext ]
authorityKeyIdentifier=keyid:always,issuer:always
EOF

cd ..
openssl ca -revoke revoked_server.cert.pem -cert ca.cert.pem -keyfile ca.key.pem -config crl/crl_openssl.conf
openssl ca -revoke revoked_client.cert.pem -cert ca.cert.pem -keyfile ca.key.pem -config crl/crl_openssl.conf

openssl ca -config crl/crl_openssl.conf -gencrl -out root.crl.pem  -cert ca.cert.pem -keyfile ca.key.pem

openssl crl -in root.crl.pem -noout -text

*/

// clang-format on

// ca.cert.pem
constexpr const char* kCaCertPem = R"(-----BEGIN CERTIFICATE-----
MIIFkTCCA3mgAwIBAgIUcMw9rJiEZSmS7rE6kxS7sXvkBg8wDQYJKoZIhvcNAQEL
BQAwQjELMAkGA1UEBhMCQVUxDTALBgNVBAgMBFNvbWUxEzARBgNVBAcMClNvbWUg
T3RoZXIxDzANBgNVBAMMBlJvb3RDQTAeFw0yMjAzMjQxMjA4MDBaFw00MjAzMTkx
MjA4MDBaMEIxCzAJBgNVBAYTAkFVMQ0wCwYDVQQIDARTb21lMRMwEQYDVQQHDApT
b21lIE90aGVyMQ8wDQYDVQQDDAZSb290Q0EwggIiMA0GCSqGSIb3DQEBAQUAA4IC
DwAwggIKAoICAQDqiud+M/IUfkAle22I/vSp75+jVI/MQdnXLX0tX0Et3X2GnoW0
0Qz01cO0AQPy1h2wSu1UNbNXZlz/oLnslJ2XYVoGNM7Scmw/fSIGe8ci7k36D8zY
WsWfvgwLP/FGRycmIIdP49Hr4vS0i/Y05q04H7v33pPi4KKdKg4rb8roxAJ8Dx1K
qCERGZWYxXa7loJlBBSiwBdc0ZZpMLcL0dmDBd99MBeUCCj0cvNOlQ/u/RN/206T
ABRxyOH22Hydmc55mDnYvYNYGz9VGVufTLFvgrFjG3hK/aGXtMjpkEtDJrS5W3hx
JXZXic5l5a3u+3DStyIaKoIQCAC5Wil0iM/Q/C6tQhIa/+OtA/Q+PUnkqSeonGkV
wu/ZPUeh1g489qdDMxrwkRNwOC2GPTN2NaX2wZpGTJG16bT8TuDnBKKHavQ7XQRR
Yuf5+0lIA6IP+gz79CgCZ5GlJRECa/gj5JDn0LnUPoRJU2AYnsISRjCidQfPurmB
fMU+Rub5rKd6GY6vyO+UbvNk1PVU6k1eYA1v50tyna60sRVLqsstEy1he56Ux5D4
FakHDYZ9R9L7X7d2Vnxb2+3IAYbQU8Qfa6n3uwYoXjLp6pgeKpGqMyPCLjooMjaP
xNotWlsO+mhf2WYNq4Lt49df/GvkIm0WMLRjECvxzhGEkSCDIFCvuAYV2QIDAQAB
o38wfTAdBgNVHQ4EFgQUxVc5EFgPQLYM/oxvjj/emsjpnnYwHwYDVR0jBBgwFoAU
xVc5EFgPQLYM/oxvjj/emsjpnnYwDwYDVR0TAQH/BAUwAwEB/zAqBgNVHR8EIzAh
MB+gHaAbhhlodHRwOi8vWzo6MV06MTMwOC9jcmwucGVtMA0GCSqGSIb3DQEBCwUA
A4ICAQDHrTZ94NIVlZ7r261JuFs/YUUab2XYZstBcDGZFl2FJ0+UWk9gYUVfIEl1
9Bk8ZC/TuLBNkggw/CRMPwZaDIqdCNs8/Q6kWfLTeSouGwUqXpojwjDPWnYoNtAl
jR4+ds5TAAdHmuSuBVJg4FDegGCbBBk/mtyGMO8qHKI6Cw03BHG8grYwPJU1tnmd
/nnXNEsNIc92yg/hGcmPlh2C75gYEWRlywP4dUr8+xs7CKi7sFCaFcg8K+AMRcA8
xDBmg1xPsQ4xVcD3GEGQz/BNyeMlgU0L3xtko8HezuYzhf3JBAoi8Cg6j5VPfscG
OnhU2CSZN6FhuHPv9iEsFO94TnbdnSD0Q2RzBoazojkdSL/pxiNVlxytxxNvuRLh
09o8LID9wqpmBt7CPr0ZHFzuXjukMt7qEfhjHMpC0SBgKE9rKEuGgUL6e2uMYvyi
fVSIVd8t+A9upC7X2e3uJVT4RMKNXd/qAc72FVHGw/Tu8dQxhfiSIzIy2miSYUqH
QIJPBu3zXuweU6yBMVb32Z6cCxcVYWjKfpZPgRPD6ObutyrFQ4W3eZ+QG/U6arDh
/eBHRJ2OdEXPUX7VS5Ktxnoa3HLXUFrWzGqxTnR+QmgEyvpdZRyLDqyHHbomV7cR
7Bka9Eu8bcqFDW/APytFStIaXpi15vdCimdSFk7x6vINRn3hfA==
-----END CERTIFICATE-----)";

// revoked_server.pem
constexpr const char* kRevokedServerPrivateKey =
    R"(-----BEGIN PRIVATE KEY-----
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
E5yRYARqFgC4Bhy046Gg5qwfx04nHg==
-----END PRIVATE KEY-----)";

// revoked_client.pem
constexpr const char* kRevokedClientPrivateKey =
    R"(-----BEGIN PRIVATE KEY-----
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
jn88HM1mBulDWvwj/yLWR7feA7ATFQ==
-----END PRIVATE KEY-----)";

// revoked_server.cert.pem
constexpr const char* kServerCertificate =
    R"(-----BEGIN CERTIFICATE-----
MIIE9TCCAt0CAQEwDQYJKoZIhvcNAQELBQAwQjELMAkGA1UEBhMCQVUxDTALBgNV
BAgMBFNvbWUxEzARBgNVBAcMClNvbWUgT3RoZXIxDzANBgNVBAMMBlJvb3RDQTAe
Fw0yMjAzMjQxMjA4MDBaFw00MTEyMDkxMjA4MDBaMD8xCzAJBgNVBAYTAkFVMQ0w
CwYDVQQIDARTb21lMRMwEQYDVQQHDApTb21lIE90aGVyMQwwCgYDVQQDDAM6OjEw
ggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDzeYAV65lml1Lw0VZdGGn4
AVulG06s2RrVBMtAurcebHlEPK1VJDXT2mLxsH+6ajAmio2OaILZ7R/cbOmF3ubb
5P+xUNcULms4EeYmflZsuduk92vZXUqZx9U45ZdAiOmKCm/NPnAbsKT3TUoNuZFT
imuuX4MxVmQUIXg687aG1pcc2zWhvzflSIn59zsJ3mBxrInqxOhQkjD6xBQoAlwt
JKx73Gyd4yMWL4jywHIDAmkCWze+xtlFjXEes0h0ufDwOweJMXoAum58c4BWhmOg
qMQP9eC4TCveKUbHAGXPP896vqpsI09aZuZ4rluFFK3yZLAiOt5CnVuY9jV93mPl
bEz8s+kWSCSdC238YjYEfM4weZ2aNEJ154A6ca7/3kXWIlFmeJ4JuZRpA04uh7iu
GXnI6ySCZR/I+KQZkvY7q3h+xn48GmkhE422v/UEbQJOFv93DQKqSbvZAlBYBMlu
PZ6sHhIfX9v8ZGQLvoj1fsqjzg6kpz8Q2yCo+C7u3h1s26wyf4pSatIij4fE/yLG
3m6FQixS9lDftV4iffkZpo7sjecYErv0aQxH2uAJtHndiw3/wgT1mCtSKlX4htjz
cDSQYC8fAGu1w2GYazxLeYFOf/L0xBweGqF0Ty88EuiZcOlclXef/5rMqlEV5ssh
BE9STQWrM8bEj8goAXnyyQIDAQABMA0GCSqGSIb3DQEBCwUAA4ICAQCS0Xk8+QBt
T/12GkGgegSzHxWwsWVFEylxb95NqzBnK0ZAP3TSTmpylxCEJ3M+8upxzBfvC6dn
9W8LFGi7SmZhweuyVaoFWYYcu9KwDQtvcoD/mmAHoeC9DQk/H54NMhvpCUF6fIXA
fptQEOPn+nXkJqMO3sHrLuJ7/+j75inBqiHZKZEGpj8gJHzS6VHkjmsI2W1uphHL
FyGUAa7ZPiRa2ouMqfIwRnAycxDIIyYFVK6k9sTOwss/q65Li/h5EqguTFMPCaIE
vKqOzPb1VVkmWZAP9/MpXXsManmLRoEhYexMxmogNkE6nC1RMwOVjthb2YAv+P3B
BdhblF5zTHSXdP0hZQnyx0SxcCHk/negOp0UX4tf/4xj1+BgknDQPz5JE8efq+VQ
dh9FpMF2DX3UtzfSDppESGiSEggrYZXM/nZmqYJZf0un/GTX51AYAy+OfOHsx+L/
+sx5wG8bLM4xAXakPQvQJXEcDQSPMFlWWQqzhYdNg5lg//nIfezvaEQ9NoqM/bzb
gDaLq5Du7IOAOQQ0jtekOtDF67fqPM/7iODA31+Z1SNgsixcUFSYWkJn6M87Y7Rc
lk6zGNFhRo6EzGEb0eWceWbKC/k2/Gdu9ZIqk5YDoNYM3ek/uy1LmIyVqnzH/75a
SeLUuMSc9IsCOxPtCUGEQ2KWW51oHQ+XZA==
-----END CERTIFICATE-----)";

// revoked_client.cert.pem
constexpr const char* kClientCertificate =
    R"(-----BEGIN CERTIFICATE-----
MIIE+DCCAuACAQIwDQYJKoZIhvcNAQELBQAwQjELMAkGA1UEBhMCQVUxDTALBgNV
BAgMBFNvbWUxEzARBgNVBAcMClNvbWUgT3RoZXIxDzANBgNVBAMMBlJvb3RDQTAe
Fw0yMjAzMjQxMjA4MDFaFw00MTEyMDkxMjA4MDFaMEIxCzAJBgNVBAYTAkFVMQ0w
CwYDVQQIDARTb21lMRMwEQYDVQQHDApTb21lIE90aGVyMQ8wDQYDVQQDDAZjbGll
bnQwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQCQmeV4PLr/D/O3QSUx
9596hd22+ja5nz+6MBkoq008qD5gxNeCgyO4xzFo1UZTT9eKi2wl+pDNbxRz2jbl
psEsRZOIwIwLBbcRcWS+rbmsZ9wYBgIfsLuoOgOAYyYsOzdvtS3GQnNiayzAISHq
WsAKOv+IshDoNgMYIYUXUQDSUMi7f3UtvQF1YlRvTn0s8qa4gl7Y5qNLPWVzEpJN
92sqUr0wvCG0VhHLuSb7IZgyoz5EHuqqvLxV++yXEBsxiyUA7VUBCOAt4xgx8jtZ
fSrZJTBlXxwXJ2X9vhoo9M0pdrAPuY1HzYffki3Z7S/AQ0jpOuy/rFd45JQ/6D4Q
axZhj7zWpKVEeIZmfylLFIrljIyqtgFr8gsApMIsqN3w817xwXlC/IWAd5hKby3U
cfrLP8PJCuXQOH1ulVxdPndGDfs9dXkTbDuR/6va3RCZbHUqi6e44HGneRc5UNZm
FD7KPs5GbFq9yBSc7T4zYi4RX4kR4EyLCO7wN6hB4BZT8mo3CABvkdgHWjPmwsNf
y2z6L+iC4ai6oao2Qkoc6fEX2MVYYPNRCSm6WTkhhBJVM/6J7EqtcgG49lf8LNZm
eV7YT8xZaVEdR63EWiR0FKKbOTrGfN9qTC4L/tdIpqPStNuRFknR4uOi33ND5KWn
Ul7ke7gctbsquhbZtMFcomHFOwIDAQABMA0GCSqGSIb3DQEBCwUAA4ICAQC61sFh
gFPsYfLzFp/fojskOMDXtQlrf8FvMNwdHq8EtlkWOVF7d/q7aMFsHB2ajVPw/9A/
nehyEhCOmuepu3sxuOAWrVJwso/vHD7gztH8YT6X7rkjz+6UMGN59WZpJHl3R67s
5y/etGMEQDKD15B6wPcQloiw9EJYUTswlXQ7E/uHFDz3Ues00mIAZyXzqQ5VWwLN
zJhKdO2NQZb0c9/W4VGeZ4U4uHVI9dpFdjoLzzwIRTEyXLXZCVw5zh2NIkhXJoRj
e3QgjgBBYrGcDs5PBuyPgRa4G85valjy2+fYZfCLFAcYYMdgQsD9dqJj7h0GUD8S
zLy4sAxNzZK2evdNyxGvqUOGpiuT29F9XO4sLH38f0QdYfuEAwQnsAAo0/rk1eom
tOVRDq8ddTAjKEzob3VbVRxQlVkVb6PwLcugfft30YxUX/CS58FzLonS5Antu20Y
sOKzhCyLsy7+pxs9SaT6YO+ZWRXGuUVgargKhhJq2ya3x1wRH3ZId4rAmA8Uqqjx
y60lJkbtoIfdl1RIvPGkgmgPAgxMN645GUWQEN5/wv+UH4QLy7/uAbezWLcD2KuX
FbZwEv2PS7/GQ6/GFOGAvu2PsghpvzFASvfWQc8pLGkJ7yBWaEwwt4E/d0sngbRv
BN36E+XgFsnNpPYywIlUlwJqz8Ptnf6CG2MLpw==
-----END CERTIFICATE-----)";

// root.crl.pem
constexpr const char* kCrlFile = R"(-----BEGIN X509 CRL-----
MIICxTCBrgIBATANBgkqhkiG9w0BAQsFADBCMQswCQYDVQQGEwJBVTENMAsGA1UE
CAwEU29tZTETMBEGA1UEBwwKU29tZSBPdGhlcjEPMA0GA1UEAwwGUm9vdENBFw0y
MjAzMjQxMjA4MDFaFw0zMDA2MTAxMjA4MDFaMCgwEgIBARcNMjIwMzI0MTIwODAx
WjASAgECFw0yMjAzMjQxMjA4MDFaoA4wDDAKBgNVHRQEAwIBADANBgkqhkiG9w0B
AQsFAAOCAgEArMXjTF/ZfY2X+XeXHpGcQZD1Z+1L1zBlfhaoFtvcY3oXQIGibZuj
8LqXNCnDc3dtUwD1WOkA7WamR0XA1P3GzqAjfC01Hx+T5E4W3xNqrcz+dtQVWqeb
Rcz4YnzZ9EXHgkw2bJ42lbUEnJxCf2D1LhV9Joj11wCqx3pHyJMEoHMQ/BLrnIJT
rPKic6fkpPJzJxnQt1njXx4FK3afECImv46cx2LW/+0dCzA43vnq9Qi6y0rkAaUt
3R86IU5KoPCzrjRYNXn/gIWsurOhd6FJqwZRBjugCD4AgUPyQI6xkQf0gtqFdWNC
jMCiBIKEGIhvKVwy0JR26jlFueyTtKtB8c8gqsPfs4vFsVpPn5LTzRjQsGz4rO+Z
QMJc9WyHTj+I/Oc/rDQZ4QsXRTLxPqd6ZZVPQ8wl20uP2gG/BV/KkkukhyIO21iT
ZKnbWz2lJoe/VlsqhOSivX/v/4OS1zA+x2utGimsczMVXrSd47l7Jqt2U+gYfHjc
KYQCiwEGnhw4Hv7Frs6HXFuYzT+9gEa950bEjCM6uGts78O2fp8/K3163/AHAg3M
HH308s3RDANtD/ytHcnLjZZD4rJEgz9uaP1/8eEzNTeopH72mX3W8MbtcD+ntv+/
EmOKfeOntrWGKRoDws82ckOkpBkZ0/9gsl8g18u+jFCcSUfmXH7FtGg=
-----END X509 CRL-----)";

struct TlsServer {
  TlsServer() : port_(tcp_listener_.socket.Getsockname().Port()) {}

  void ReceiveAndShutdown(std::initializer_list<crypto::Certificate> cas = {}) {
    auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    auto socket = tcp_listener_.socket.Accept(deadline);

    auto tls_server = engine::io::TlsWrapper::StartTlsServer(
        std::move(socket),
        crypto::Certificate::LoadFromString(kServerCertificate),
        crypto::PrivateKey::LoadFromString(kRevokedServerPrivateKey), deadline,
        cas);

    std::array<char, 2048> data{};
    const auto size = tls_server.RecvSome(data.data(), data.size(), deadline);
    EXPECT_GT(size, 0);

    LOG_INFO() << "HTTPS Server receive: " << data.data();

    constexpr std::string_view kResp =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "OK";
    EXPECT_GT(tls_server.SendAll(kResp.data(), kResp.size(), deadline), 0);
    socket = tls_server.StopTls(deadline);
    EXPECT_TRUE(socket.IsValid());
  }

  internal::net::TcpListener tcp_listener_;
  int port_;
};

auto InterceptCrlDistribution() {
  engine::io::Socket listener{engine::io::AddrDomain::kInet6,
                              engine::io::SocketType::kStream};
  engine::io::Sockaddr addr;
  auto* sa = addr.template As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin6_port = htons(kCrlDistributionPort);
  listener.Bind(addr);

  return engine::AsyncNoSpan([listener = std::move(listener)]() mutable {
    while (!engine::current_task::IsCancelRequested()) {
      auto socket = listener.Accept({});

      ASSERT_FALSE(socket.IsValid())
          << "X509v3 CRL was requested while working with the certificate";
    }
  });
}

}  // namespace

UTEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithNoCrl)) {
  (void)kCrlFile;
  auto task = InterceptCrlDistribution();

  auto pkey = crypto::PrivateKey::LoadFromString(kRevokedClientPrivateKey, "");
  auto cert = crypto::Certificate::LoadFromString(kClientCertificate);
  auto ca = crypto::Certificate::LoadFromString(kCaCertPem);

  auto http_client_ptr = utest::CreateHttpClient();
  TlsServer tls_server;
  const auto ssl_url = fmt::format("https://[::1]:{}", tls_server.port_);

  auto response_future = http_client_ptr->CreateRequest()
                             ->post(ssl_url)
                             ->timeout(utest::kMaxTestWaitTime)
                             ->ca(ca)
                             ->client_key_cert(pkey, cert)
                             ->verify(true)
                             ->async_perform();

  tls_server.ReceiveAndShutdown({ca});
  response_future.Wait();
  auto resp = response_future.Get();
  EXPECT_TRUE(resp->IsOk());
  EXPECT_EQ(resp->body_view(), "OK");
}

UTEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithCrl)) {
  auto tmp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(tmp_file.GetPath(), kCrlFile);

  auto task = InterceptCrlDistribution();

  auto pkey = crypto::PrivateKey::LoadFromString(kRevokedClientPrivateKey, "");
  auto cert = crypto::Certificate::LoadFromString(kClientCertificate);
  auto ca = crypto::Certificate::LoadFromString(kCaCertPem);

  auto http_client_ptr = utest::CreateHttpClient();
  TlsServer tls_server;
  const auto ssl_url = fmt::format("https://[::1]:{}", tls_server.port_);

  auto response_future = http_client_ptr->CreateRequest()
                             ->post(ssl_url)
                             ->timeout(utest::kMaxTestWaitTime)
                             ->ca(ca)
                             ->crl_file(tmp_file.GetPath())
                             ->client_key_cert(pkey, cert)
                             ->verify(true)
                             ->async_perform();

  UEXPECT_THROW(tls_server.ReceiveAndShutdown({ca}), engine::io::TlsException);
  response_future.Wait();
  UEXPECT_THROW(response_future.Get(), clients::http::SSLException);
}

UTEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithCrlNoVerify)) {
  auto tmp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(tmp_file.GetPath(), kCrlFile);

  auto task = InterceptCrlDistribution();

  auto pkey = crypto::PrivateKey::LoadFromString(kRevokedClientPrivateKey, "");
  auto cert = crypto::Certificate::LoadFromString(kClientCertificate);
  auto ca = crypto::Certificate::LoadFromString(kCaCertPem);

  auto http_client_ptr = utest::CreateHttpClient();
  TlsServer tls_server;
  const auto ssl_url = fmt::format("https://[::1]:{}", tls_server.port_);

  auto response_future = http_client_ptr->CreateRequest()
                             ->post(ssl_url)
                             ->timeout(utest::kMaxTestWaitTime)
                             ->ca(ca)
                             ->crl_file(tmp_file.GetPath())
                             ->client_key_cert(pkey, cert)
                             ->verify(false)  // do not do that in production!
                             ->async_perform();

  UEXPECT_NO_THROW(tls_server.ReceiveAndShutdown({ca}));
  response_future.Wait();
  UEXPECT_NO_THROW(response_future.Get());
}

UTEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithNoServerCa)) {
  (void)kCrlFile;
  auto task = InterceptCrlDistribution();

  auto pkey = crypto::PrivateKey::LoadFromString(kRevokedClientPrivateKey, "");
  auto cert = crypto::Certificate::LoadFromString(kClientCertificate);
  auto ca = crypto::Certificate::LoadFromString(kCaCertPem);

  auto http_client_ptr = utest::CreateHttpClient();
  TlsServer tls_server;
  const auto ssl_url = fmt::format("https://[::1]:{}", tls_server.port_);

  auto response_future = http_client_ptr->CreateRequest()
                             ->post(ssl_url)
                             ->timeout(utest::kMaxTestWaitTime)
                             ->ca(ca)
                             ->client_key_cert(pkey, cert)
                             ->verify(true)
                             ->async_perform();

  tls_server.ReceiveAndShutdown();
  response_future.Wait();
  auto resp = response_future.Get();
  EXPECT_TRUE(resp->IsOk());
  EXPECT_EQ(resp->body_view(), "OK");
}

UTEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithNoClientCa)) {
  auto task = InterceptCrlDistribution();
  auto pkey = crypto::PrivateKey::LoadFromString(kRevokedClientPrivateKey, "");
  auto cert = crypto::Certificate::LoadFromString(kClientCertificate);

  auto http_client_ptr = utest::CreateHttpClient();
  TlsServer tls_server;
  const auto ssl_url = fmt::format("https://[::1]:{}", tls_server.port_);

  auto response_future = http_client_ptr->CreateRequest()
                             ->post(ssl_url)
                             ->timeout(utest::kMaxTestWaitTime)
                             ->client_key_cert(pkey, cert)
                             ->verify(true)
                             ->async_perform();

  auto ca = crypto::Certificate::LoadFromString(kCaCertPem);
  UEXPECT_THROW(tls_server.ReceiveAndShutdown({ca}), engine::io::TlsException);
  response_future.Wait();
  UEXPECT_THROW(response_future.Get(), clients::http::SSLException);
}

USERVER_NAMESPACE_END
