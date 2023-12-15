#include <gtest/gtest.h>

#include <string_view>

#include <userver/crypto/certificate.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const std::string_view kRsaCertPem = R"(-----BEGIN CERTIFICATE-----
MIIDHTCCAgWgAwIBAgIUdeC/2p5YeWqKXWWfIWXVidqA2dUwDQYJKoZIhvcNAQEL
BQAwHjEcMBoGA1UEAwwTdXNlcnZlci1jcnlwdG8tdGVzdDAeFw0yMzEyMDEwMDAw
MDBaFw00MzA4MTgwMDAwMDBaMB4xHDAaBgNVBAMME3VzZXJ2ZXItY3J5cHRvLXRl
c3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCjTeIOQqFQ5WXe3epA
xyDFUN11yQFu4Bz6DmFkPjuNjBxMHVeZ2dTHpjFG5sSokQUG+OSwJ3M98jlXyvgQ
bDONhzGvORH55e8I3+OuYQN/i7HcUUDhIWc/OmnyO2/DUI/E2/uLANCDC5cBgjpe
ysxiAxaJZoRBK0Gq0VIubJbAVRrtgMgby2J0JLFYyPnPa3rIElJwNZEX5a2+PnwO
zeRqWTGTIUPVaxMrxu9Ti/1lGAUg9DCzbg36VALIbnnMhc1CINmGpNAXv611M1tv
bSrk8qpXW6HY7N/ezX/TW4UIfHpBXGuR1zSkFqGEJ/cgiOigdNtyjjwobIvRrO40
zhP5AgMBAAGjUzBRMB0GA1UdDgQWBBSw8zB5AvB60Pq3ayUe9Unoaa3gujAfBgNV
HSMEGDAWgBSw8zB5AvB60Pq3ayUe9Unoaa3gujAPBgNVHRMBAf8EBTADAQH/MA0G
CSqGSIb3DQEBCwUAA4IBAQCZPthv9VMkK+c7bjqo1PAMT8NAUSvAGnt97eKBl4E0
tJVhFQWe52QkIxZLhTg6KgBZa5JxoL3Lgat2oT+WH15ebghp+uzjSs+j/XWESrme
BQaTdWpi66RnB0sFnZ5KkDXKoLwz2eLY53p8rDuDrukGAhu9rKsmPlINgRICjSL6
AKe1Kl8BJ6XLnxfHps7gutUGcSatpKP0vaN3BnYEnNeQ4jDqTOeRgujGoDYCkAoX
vnt5k03sADG1HQMJJ+okTNhM3X0nbmxSxQw3arVzkTtkY39zGPqQxKgDch2uCzEv
iW5OwYvGErHvYQaO0LtwjzO8LamystYgUIXVV+fFL3w6
-----END CERTIFICATE-----
)";

constexpr const std::string_view kEcCertPem = R"(-----BEGIN CERTIFICATE-----
MIIBzTCCAVSgAwIBAgIUcnh0lUsE42M7/lU/URAA7Qy7cDIwCgYIKoZIzj0EAwIw
HjEcMBoGA1UEAwwTdXNlcnZlci1jcnlwdG8tdGVzdDAeFw0yMzEyMDEwMDAwMDBa
Fw00MzA4MTgwMDAwMDBaMB4xHDAaBgNVBAMME3VzZXJ2ZXItY3J5cHRvLXRlc3Qw
djAQBgcqhkjOPQIBBgUrgQQAIgNiAAQS50yLyWGfqOO6BWHWSss+h29BP2TOgcgc
Uy7pRy5/toMHU9u9Ku81WawfgFzKVVd4TIgL0gIjta3pGxg4DKXcNEF8QjImOCZi
Xe5xSc0+6VOJ082fMvaLrX0YcYUPvjmjUzBRMB0GA1UdDgQWBBRtHoWnxXcNoqJu
ZdYJzaRnQGFF5jAfBgNVHSMEGDAWgBRtHoWnxXcNoqJuZdYJzaRnQGFF5jAPBgNV
HRMBAf8EBTADAQH/MAoGCCqGSM49BAMCA2cAMGQCMGBPy5JHFnIqYrmYZp06C9wC
2LWO59kL/d3z6Yt/BuzFB69kUah5cQLbflIRQHoGWgIwQTbw6jqxRlAfKhMtosmp
gRUIzzXiJrXsfaQHv6XY7lf6kHluk7WZXZXicQEX58sv
-----END CERTIFICATE-----
)";

}  // namespace

TEST(Crypto, CertificatePemDefault) {
  EXPECT_FALSE(crypto::Certificate{}.GetPemString().has_value());
}

TEST(Crypto, CertificatePemRoundtripRsa) {
  const auto rsa_cert = crypto::Certificate::LoadFromString(kRsaCertPem);
  EXPECT_EQ(rsa_cert.GetPemString(), kRsaCertPem);
}

TEST(Crypto, CertificatePemRoundtripEc) {
  const auto ec_cert = crypto::Certificate::LoadFromString(kEcCertPem);
  EXPECT_EQ(ec_cert.GetPemString(), kEcCertPem);
}

USERVER_NAMESPACE_END
