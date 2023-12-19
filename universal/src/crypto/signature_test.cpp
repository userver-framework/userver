#include <gtest/gtest.h>

#include <string_view>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/crypto/verifiers.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/flags.hpp>

#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>

#include <fstream>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto rsa512_priv_key = R"(-----BEGIN RSA PRIVATE KEY-----
MIICWwIBAAKBgQDdlatRjRjogo3WojgGHFHYLugdUWAY9iR3fy4arWNA1KoS8kVw
33cJibXr8bvwUAUparCwlvdbH6dvEOfou0/gCFQsHUfQrSDv+MuSUMAe8jzKE4qW
+jK+xQU9a03GUnKHkkle+Q0pX/g6jXZ7r1/xAK5Do2kQ+X5xK9cipRgEKwIDAQAB
AoGAD+onAtVye4ic7VR7V50DF9bOnwRwNXrARcDhq9LWNRrRGElESYYTQ6EbatXS
3MCyjjX2eMhu/aF5YhXBwkppwxg+EOmXeh+MzL7Zh284OuPbkglAaGhV9bb6/5Cp
uGb1esyPbYW+Ty2PC0GSZfIXkXs76jXAu9TOBvD0ybc2YlkCQQDywg2R/7t3Q2OE
2+yo382CLJdrlSLVROWKwb4tb2PjhY4XAwV8d1vy0RenxTB+K5Mu57uVSTHtrMK0
GAtFr833AkEA6avx20OHo61Yela/4k5kQDtjEf1N0LfI+BcWZtxsS3jDM3i1Hp0K
Su5rsCPb8acJo5RO26gGVrfAsDcIXKC+bQJAZZ2XIpsitLyPpuiMOvBbzPavd4gY
6Z8KWrfYzJoI/Q9FuBo6rKwl4BFoToD7WIUS+hpkagwWiz+6zLoX1dbOZwJACmH5
fSSjAkLRi54PKJ8TFUeOP15h9sQzydI8zJU+upvDEKZsZc/UhT/SySDOxQ4G/523
Y0sz/OZtSWcol/UMgQJALesy++GdvoIDLfJX5GBQpuFgFenRiRDabxrE9MNUZ2aP
FaFp+DyAe+b4nDwuJaW2LURbr8AEZga7oQj0uYxcYw==
-----END RSA PRIVATE KEY-----)";
constexpr auto rsa512_pub_key = R"(-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDdlatRjRjogo3WojgGHFHYLugd
UWAY9iR3fy4arWNA1KoS8kVw33cJibXr8bvwUAUparCwlvdbH6dvEOfou0/gCFQs
HUfQrSDv+MuSUMAe8jzKE4qW+jK+xQU9a03GUnKHkkle+Q0pX/g6jXZ7r1/xAK5D
o2kQ+X5xK9cipRgEKwIDAQAB
-----END PUBLIC KEY-----)";
constexpr auto rsa512_pub_key_invalid = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxzYuc22QSst/dS7geYYK
5l5kLxU0tayNdixkEQ17ix+CUcUbKIsnyftZxaCYT46rQtXgCaYRdJcbB3hmyrOa
vkhTpX79xJZnQmfuamMbZBqitvscxW9zRR9tBUL6vdi/0rpoUwPMEh8+Bw7CgYR0
FK0DhWYBNDfe9HKcyZEv3max8Cdq18htxjEsdYO0iwzhtKRXomBWTdhD5ykd/fAC
VTr4+KEY+IeLvubHVmLUhbE5NgWXxrRpGasDqzKhCTmsa2Ysf712rl57SlH0Wz/M
r3F7aM9YpErzeYLrl0GhQr9BVJxOvXcVd4kmY+XkiCcrkyS1cnghnllh+LCwQu1s
YwIDAQAB
-----END PUBLIC KEY-----)";

constexpr auto rsa2048_priv_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQC4ZtdaIrd1BPIJ
tfnF0TjIK5inQAXZ3XlCrUlJdP+XHwIRxdv1FsN12XyMYO/6ymLmo9ryoQeIrsXB
XYqlET3zfAY+diwCb0HEsVvhisthwMU4gZQu6TYW2s9LnXZB5rVtcBK69hcSlA2k
ZudMZWxZcj0L7KMfO2rIvaHw/qaVOE9j0T257Z8Kp2CLF9MUgX0ObhIsdumFRLaL
DvDUmBPr2zuh/34j2XmWwn1yjN/WvGtdfhXW79Ki1S40HcWnygHgLV8sESFKUxxQ
mKvPUTwDOIwLFL5WtE8Mz7N++kgmDcmWMCHc8kcOIu73Ta/3D4imW7VbKgHZo9+K
3ESFE3RjAgMBAAECggEBAJTEIyjMqUT24G2FKiS1TiHvShBkTlQdoR5xvpZMlYbN
tVWxUmrAGqCQ/TIjYnfpnzCDMLhdwT48Ab6mQJw69MfiXwc1PvwX1e9hRscGul36
ryGPKIVQEBsQG/zc4/L2tZe8ut+qeaK7XuYrPp8bk/X1e9qK5m7j+JpKosNSLgJj
NIbYsBkG2Mlq671irKYj2hVZeaBQmWmZxK4fw0Istz2WfN5nUKUeJhTwpR+JLUg4
ELYYoB7EO0Cej9UBG30hbgu4RyXA+VbptJ+H042K5QJROUbtnLWuuWosZ5ATldwO
u03dIXL0SH0ao5NcWBzxU4F2sBXZRGP2x/jiSLHcqoECgYEA4qD7mXQpu1b8XO8U
6abpKloJCatSAHzjgdR2eRDRx5PMvloipfwqA77pnbjTUFajqWQgOXsDTCjcdQui
wf5XAaWu+TeAVTytLQbSiTsBhrnoqVrr3RoyDQmdnwHT8aCMouOgcC5thP9vQ8Us
rVdjvRRbnJpg3BeSNimH+u9AHgsCgYEA0EzcbOltCWPHRAY7B3Ge/AKBjBQr86Kv
TdpTlxePBDVIlH+BM6oct2gaSZZoHbqPjbq5v7yf0fKVcXE4bSVgqfDJ/sZQu9Lp
PTeV7wkk0OsAMKk7QukEpPno5q6tOTNnFecpUhVLLlqbfqkB2baYYwLJR3IRzboJ
FQbLY93E8gkCgYB+zlC5VlQbbNqcLXJoImqItgQkkuW5PCgYdwcrSov2ve5r/Acz
FNt1aRdSlx4176R3nXyibQA1Vw+ztiUFowiP9WLoM3PtPZwwe4bGHmwGNHPIfwVG
m+exf9XgKKespYbLhc45tuC08DATnXoYK7O1EnUINSFJRS8cezSI5eHcbQKBgQDC
PgqHXZ2aVftqCc1eAaxaIRQhRmY+CgUjumaczRFGwVFveP9I6Gdi+Kca3DE3F9Pq
PKgejo0SwP5vDT+rOGHN14bmGJUMsX9i4MTmZUZ5s8s3lXh3ysfT+GAhTd6nKrIE
kM3Nh6HWFhROptfc6BNusRh1kX/cspDplK5x8EpJ0QKBgQDWFg6S2je0KtbV5PYe
RultUEe2C0jYMDQx+JYxbPmtcopvZQrFEur3WKVuLy5UAy7EBvwMnZwIG7OOohJb
vkSpADK6VPn9lbqq7O8cTedEHttm6otmLt8ZyEl3hZMaL3hbuRj6ysjmoFKx6CrX
rK0/Ikt5ybqUzKCMJZg2VKGTxg==
-----END PRIVATE KEY-----)";
constexpr auto rsa2048_pub_key = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuGbXWiK3dQTyCbX5xdE4
yCuYp0AF2d15Qq1JSXT/lx8CEcXb9RbDddl8jGDv+spi5qPa8qEHiK7FwV2KpRE9
83wGPnYsAm9BxLFb4YrLYcDFOIGULuk2FtrPS512Qea1bXASuvYXEpQNpGbnTGVs
WXI9C+yjHztqyL2h8P6mlThPY9E9ue2fCqdgixfTFIF9Dm4SLHbphUS2iw7w1JgT
69s7of9+I9l5lsJ9cozf1rxrXX4V1u/SotUuNB3Fp8oB4C1fLBEhSlMcUJirz1E8
AziMCxS+VrRPDM+zfvpIJg3JljAh3PJHDiLu902v9w+Iplu1WyoB2aPfitxEhRN0
YwIDAQAB
-----END PUBLIC KEY-----)";
constexpr auto rsa2048_pub_key_invalid = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxzYuc22QSst/dS7geYYK
5l5kLxU0tayNdixkEQ17ix+CUcUbKIsnyftZxaCYT46rQtXgCaYRdJcbB3hmyrOa
vkhTpX79xJZnQmfuamMbZBqitvscxW9zRR9tBUL6vdi/0rpoUwPMEh8+Bw7CgYR0
FK0DhWYBNDfe9HKcyZEv3max8Cdq18htxjEsdYO0iwzhtKRXomBWTdhD5ykd/fAC
VTr4+KEY+IeLvubHVmLUhbE5NgWXxrRpGasDqzKhCTmsa2Ysf712rl57SlH0Wz/M
r3F7aM9YpErzeYLrl0GhQr9BVJxOvXcVd4kmY+XkiCcrkyS1cnghnllh+LCwQu1s
YwIDAQAB
-----END PUBLIC KEY-----)";

constexpr auto ecdsa256v1_priv_key = R"(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgPGJGAm4X1fvBuC1z
SpO/4Izx6PXfNMaiKaS5RUkFqEGhRANCAARCBvmeksd3QGTrVs2eMrrfa7CYF+sX
sjyGg+Bo5mPKGH4Gs8M7oIvoP9pb/I85tdebtKlmiCZHAZE5w4DfJSV6
-----END PRIVATE KEY-----)";
constexpr auto ecdsa256v1_pub_key = R"(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEQgb5npLHd0Bk61bNnjK632uwmBfr
F7I8hoPgaOZjyhh+BrPDO6CL6D/aW/yPObXXm7SpZogmRwGROcOA3yUleg==
-----END PUBLIC KEY-----)";
constexpr auto ecdsa256v1_pub_key_invalid = R"(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEoBUyo8CQAFPeYPvv78ylh5MwFZjT
CLQeb042TjiMJxG+9DLFmRSMlBQ9T/RsLLc+PmpB1+7yPAR+oR5gZn3kJQ==
-----END PUBLIC KEY-----)";

constexpr auto ecdsa521p1_priv_key = R"(-----BEGIN EC PRIVATE KEY-----
MIHcAgEBBEIADZbvukRPYwxbnY0RRAm7TnXV04JS3c/afpJ7kkL6FPI9S0bOjYIy
SKoARgcENeLiqm6U2wCfgC06mo2KxRfWvYOgBwYFK4EEACOhgYkDgYYABAFDGNaa
Lwb5Ra5PbFXL0I8T8E4FHtoinKCw8Lb0g/mhY79L68Lepc9zXu0ZLcKjAn9Sb6kT
UFwYEYSBmnpmabtKbwG7CkxfvyqcCkPY84+5N1mOK+kc/uzF9/wreN+q5sj/1lLh
HIZMRqqP+mZgYVH+/DmpTGOzY/EZBKGDBc/9yClIcg==
-----END EC PRIVATE KEY-----)";
constexpr auto ecdsa521p1_pub_key = R"(-----BEGIN PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQBQxjWmi8G+UWuT2xVy9CPE/BOBR7a
IpygsPC29IP5oWO/S+vC3qXPc17tGS3CowJ/Um+pE1BcGBGEgZp6Zmm7Sm8BuwpM
X78qnApD2POPuTdZjivpHP7sxff8K3jfqubI/9ZS4RyGTEaqj/pmYGFR/vw5qUxj
s2PxGQShgwXP/cgpSHI=
-----END PUBLIC KEY-----)";
constexpr auto ecdsa521p1_pub_key_invalid = R"(-----BEGIN PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQBKlesxr9XembXLzb+woZBbcZjWDUA
wAfpnIWG/Wn6OGChEZ2UBIthj1882o6XsNhpKjFwdkTknZS39KlcLk3xre4ASAwC
WYfPaMaswhyGJNFMHdFIxk13B2fC9COe9fyQAHrGDhKJbBZQZqV8HIveVclBqXSr
M+/r5nEb+tD3mjQyRJE=
-----END PUBLIC KEY-----)";

constexpr std::string_view kSomeCert{R"(-----BEGIN CERTIFICATE-----
MIIDazCCAlOgAwIBAgIUJBjqiRVkYhB1cDMtP48iGZ7L2HgwDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMzAzMDYyMzIxMzRaFw0yNDAz
MDUyMzIxMzRaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQC2y31A/484BsC1o5MJHHBqjEwcjCvRXJG2bIv2jiLu
KsW6lSZbTDwk8fsC1O3XlAX9VmO+7pU/wEG4gvir+NTyLqfgqDAV9QAp560FgECp
I4UBrr9G/SfxKQpY1x+cglFRGtQQDqk+8Ig0Ur4Kiy1HEYe2IF+FxF/P6jS+An5v
Glp/MAjPvD/O1dbAESRsBnpl+iuSyFtT7E6BurHw7IcTbByd1mTuOQ+ezMlS6keb
fbP5eadv2UDkX5VCOz/1t7zWLhmzJ6Aypkza1WLmnA80QaNq6ugd6mLeiA+VXfJs
UuYQxaNf79nYRvmrTB4SHGrYrLb1zrw9snq2+aOw58w7AgMBAAGjUzBRMB0GA1Ud
DgQWBBRM6ULwWZBDBU3ymphBIbmMDYYVWjAfBgNVHSMEGDAWgBRM6ULwWZBDBU3y
mphBIbmMDYYVWjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCu
7WoAM8mK4s8Z2mRlzdrKF8RmPj/Ni4zrbf1ngGpge8p//wZWASIKtA2UTqUix6cz
kSoJIwuSXvN6lM/4R4LQBh4LZi1d1tpQjSE2X2Kq2tp9oct4WriRxybLA4PWMfeN
QPxds0ga5A27QKpYCtgPQf8Slz01xNCxcb7m2lMKGZIlhsfbzrnJlJ7OvG8yCBEB
Ne+djzjthXfQpX7V78T82489RALjDHxBmW5NVrFnh09DMN1MYtmAK/0oFSFoQG15
Y7g16pn3binvOwcaZri4H3tk98GfkTSBStyMIum0A/cjxgh7geanvEA6EHv1M/YM
RBy4X1eTKsAC7Qs1TQTA
-----END CERTIFICATE-----)"};

constexpr std::string_view kSomePrivKey{R"(-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC2y31A/484BsC1
o5MJHHBqjEwcjCvRXJG2bIv2jiLuKsW6lSZbTDwk8fsC1O3XlAX9VmO+7pU/wEG4
gvir+NTyLqfgqDAV9QAp560FgECpI4UBrr9G/SfxKQpY1x+cglFRGtQQDqk+8Ig0
Ur4Kiy1HEYe2IF+FxF/P6jS+An5vGlp/MAjPvD/O1dbAESRsBnpl+iuSyFtT7E6B
urHw7IcTbByd1mTuOQ+ezMlS6kebfbP5eadv2UDkX5VCOz/1t7zWLhmzJ6Aypkza
1WLmnA80QaNq6ugd6mLeiA+VXfJsUuYQxaNf79nYRvmrTB4SHGrYrLb1zrw9snq2
+aOw58w7AgMBAAECggEABBMzPqCz26bi4um7nGdi5i0sgTJqw6cRpIR09lEi3DZd
5BPxH/XqHVS7qMuAZd7p50TmwlnlP9+UV32r94F7lc6OdYaqU8L7dvOwOsAqMNsW
2vu/TeDU5B5rGaHqFX4eO/WiJp19DmckUi5KCEgkAUKVyOyd1IiVLjhlZpr9oJJn
I8PyFEgKPwB6apuHDa1dy9orBmZ/0Nsvh4DLz15eBSCNWhKW1z+2xUGvEbVK9deU
0XukA4raOBoD1drM8TTaq2GHB81xOeV+HGQ7R5fzpCVXGYniRUM1VtyK38Cws9zk
2taw9niYAVq4gldQJQxvGEJolRW6YRzenwYhsEGdQQKBgQDt6Sz8jy9wHGoxuSkZ
pSDu3gvK9XcCQaLCrD2NbroWodp2oWo8ED9ytBedH15xc+wTE9Ce9IUAvC1lAmjn
9xzg3V+onwL1shU7Pqt3STQIwALjBfKJ3ynM+GWACgSYp4Oju0R+RJu1qhUWF1mG
6zqBQzpP9j/xVPHWHOvxSsi/WwKBgQDEsYHCy04qGrnlUY+dOh6Vp9PwuRYVuUYm
QmuQxGZphfg+q6fAHMhfPmJpHC2f/Lg8qJo6Nn14PHsXkNaVq++DcYALkStRx8Ga
p35Cjkvyz+PEnvjAhO69IvU5dVjrodX3j5UntOa4E8jHiBBLCr7Xw8rq2rTo5m5V
xTOYZeScoQKBgC4MfP2hhVAQ3+kh6rehA5/YcLXnDxLru2IsznkqnnA1AcEGezaG
+pDsFMn/wVr6Ug5bPn0V3QyIX8z2S3Hinb9Ueeh8ZqlBv+RIvA2JqL2s1KI2i3xu
KnwW9CZ1SfB2M/+kBpzeELl34z70ZAq+SwOsDIRfqH7Ar1J/JJf+np6TAoGAIu9E
FMz3PRMyIUCzOUGxXPNJ+V9mRMV7aCAz6qp3ZhadWMQ0LzLtg70YzYJruBaywlNP
0TcMYeacFjtrYXoAOGBYVbWa9BN+l3FYQ4nyg1XZJnr1w3vYy08CLwpgmLlNv4KJ
A1xl/hvFdWr9OdbWvovP5pahuxvyTNmRqGewiwECgYBpbER6Q/8enu7ZWsQMQJhl
TjHpHTeC67wPly53kPortX0A7pmS8vwzk/MhJU3GTECLi2ZYIbcRq/xZltmT27ad
9dcAeHK0chrAGzEraLsVkYElGUhPkjvuuA52COmUXtX3z0WsdxAyZ5WPmsG+vV0Z
UtDIEU4gcJRnxv0O5w/XZA==
-----END PRIVATE KEY-----)"};

enum class TestFlags {
  kNone = 0,
  kSkipDigestOps = 1,
};

template <typename DsaSigner, typename DsaVerifier>
void TestDsaSignature(DsaSigner signer, DsaVerifier verifier,
                      DsaVerifier bad_verifier, std::string_view message,
                      std::string_view digest, std::string_view bad_digest,
                      utils::Flags<TestFlags> flags = {}) {
  const auto sig = signer.Sign({message});
  const auto bad_sig = signer.Sign({"bad ", message});

  EXPECT_NO_THROW(verifier.Verify({message}, sig));
  EXPECT_THROW(verifier.Verify({message}, {}), crypto::VerificationError);
  EXPECT_THROW(verifier.Verify({"not ", message}, sig),
               crypto::VerificationError);
  EXPECT_THROW(verifier.Verify({message}, bad_sig), crypto::VerificationError);

  EXPECT_THROW(bad_verifier.Verify({message}, sig), crypto::VerificationError);

  if (!(flags & TestFlags::kSkipDigestOps)) {
    const auto md_sig = signer.SignDigest(utils::encoding::FromHex(digest));
    EXPECT_THROW(signer.SignDigest(digest), crypto::SignError);
    EXPECT_THROW(signer.SignDigest(utils::encoding::FromHex(bad_digest)),
                 crypto::SignError);
    EXPECT_NO_THROW(verifier.Verify({message}, md_sig));
    EXPECT_NO_THROW(
        verifier.VerifyDigest(utils::encoding::FromHex(digest), md_sig));

    EXPECT_NO_THROW(
        verifier.VerifyDigest(utils::encoding::FromHex(digest), sig));
    EXPECT_THROW(verifier.VerifyDigest(utils::encoding::FromHex(digest), {}),
                 crypto::VerificationError);
    EXPECT_THROW(
        verifier.VerifyDigest(utils::encoding::FromHex(digest), bad_sig),
        crypto::VerificationError);
    EXPECT_THROW(verifier.VerifyDigest(digest, sig), crypto::VerificationError);
    EXPECT_THROW(
        verifier.VerifyDigest(utils::encoding::FromHex(bad_digest), sig),
        crypto::VerificationError);
    EXPECT_THROW(bad_verifier.Verify({message}, md_sig),
                 crypto::VerificationError);
    EXPECT_THROW(
        bad_verifier.VerifyDigest(utils::encoding::FromHex(digest), sig),
        crypto::VerificationError);
  }
}

}  // namespace

TEST(Crypto, SignatureNone) {
  EXPECT_TRUE(crypto::SignerNone{}.Sign({"test"}).empty());
  EXPECT_NO_THROW(crypto::VerifierNone{}.Verify({"test"}, {}));
  EXPECT_THROW(crypto::VerifierNone{}.Verify({"test"}, "test"),
               crypto::VerificationError);
}

TEST(Crypto, SignatureHs1) {
  crypto::SignerHs1 signer("secret");
  auto sig = signer.Sign({"test"});
  auto bad_sig = signer.Sign({"bad test"});
  EXPECT_EQ("1aa349585ed7ecbd3b9c486a30067e395ca4b356",
            utils::encoding::ToHex(sig));

  crypto::VerifierHs1 verifier("secret");
  EXPECT_NO_THROW(verifier.Verify({"test"}, sig));
  EXPECT_THROW(verifier.Verify({"test"}, {}), crypto::VerificationError);
  EXPECT_THROW(verifier.Verify({"not test"}, sig), crypto::VerificationError);
  EXPECT_THROW(verifier.Verify({"test"}, bad_sig), crypto::VerificationError);
}

TEST(Crypto, SignatureHs256) {
  crypto::SignerHs256 signer("secret");
  auto sig = signer.Sign({"test"});
  auto bad_sig = signer.Sign({"bad test"});
  EXPECT_EQ("0329a06b62cd16b33eb6792be8c60b158d89a2ee3a876fce9a881ebb488c0914",
            utils::encoding::ToHex(sig));

  crypto::VerifierHs256 verifier("secret");
  EXPECT_NO_THROW(verifier.Verify({"test"}, sig));
  EXPECT_THROW(verifier.Verify({"test"}, {}), crypto::VerificationError);
  EXPECT_THROW(verifier.Verify({"not test"}, sig), crypto::VerificationError);
  EXPECT_THROW(verifier.Verify({"test"}, bad_sig), crypto::VerificationError);
}

TEST(Crypto, SignatureCMSSignVerify) {
  using Signer = crypto::CmsSigner;
  using SFlags = Signer::Flags;

  using Verifier = crypto::CmsVerifier;
  using VFlags = Verifier::Flags;

  const Signer signer{crypto::Certificate::LoadFromString(kSomeCert),
                      crypto::PrivateKey::LoadFromString(kSomePrivKey)};
  const Verifier verifier{crypto::Certificate::LoadFromString(kSomeCert)};

  const auto sign_and_verify = [signer, verifier](Signer::OutForm out_form,
                                                  Verifier::InForm in_form) {
    const auto signed_data = signer.Sign(
        {"test data"}, {SFlags::kNoCerts, SFlags::kBinary, SFlags::kText},
        out_form);
    verifier.Verify({signed_data}, {VFlags::kNoSignerCertVerify}, in_form);
  };
  EXPECT_NO_THROW(
      sign_and_verify(Signer::OutForm::kSMime, Verifier::InForm::kSMime));
  EXPECT_NO_THROW(
      sign_and_verify(Signer::OutForm::kPem, Verifier::InForm::kPem));
  EXPECT_NO_THROW(
      sign_and_verify(Signer::OutForm::kDer, Verifier::InForm::kDer));
}

TEST(Crypto, SignatureCMSVerify) {
  const crypto::CmsVerifier verifier{
      crypto::Certificate::LoadFromString(kSomeCert)};

  constexpr std::string_view signed_data_base64{
      "MIICsQYJKoZIhvcNAQcCoIICojCCAp4CAQExDTALBglghkgBZQMEAgEwGQYJKoZIhvcNAQcB"
      "oAwECnRlc3QgZGF0YQoxggJtMIICaQIBATBdMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApT"
      "b21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQCFCQY6okVZGIQ"
      "dXAzLT+"
      "PIhmey9h4MAsGCWCGSAFlAwQCAaCB5DAYBgkqhkiG9w0BCQMxCwYJKoZIhvcNAQcBMBwGCSq"
      "GSIb3DQEJBTEPFw0yMzAzMTMxODI2MzFaMC8GCSqGSIb3DQEJBDEiBCAMFeiD3uhbsvNUCkf"
      "sWPYXolRxF/"
      "kJZBe6VCImgCn1ATB5BgkqhkiG9w0BCQ8xbDBqMAsGCWCGSAFlAwQBKjALBglghkgBZQMEAR"
      "YwCwYJYIZIAWUDBAECMAoGCCqGSIb3DQMHMA4GCCqGSIb3DQMCAgIAgDANBggqhkiG9w0DAg"
      "IBQDAHBgUrDgMCBzANBggqhkiG9w0DAgIBKDANBgkqhkiG9w0BAQEFAASCAQBfb+"
      "zJc7SGX83b5cIgsVly/x6ryCMtqgp158LIx8SEBEsdcufxIbcoFHLJNM/"
      "jJz6at142Fm+6FOihP8D+tzWMGxqdS8IJaCb8pJe0aOwhHeiwMrzM9+"
      "JrHEt5mJKw9wONi12ykHNnfZB5aOyqSbQK3HxoGGXrh1Hryq2h0m5DcdV+"
      "bpgPRI6wObUCzijWnQ3XeIzEHxYUsB4rh1V6E9aQu5hoMSee+f56f2PNgZBnpYcSKOIho38/"
      "3pVtOqn49Y+p2Jq+"
      "jT0dbrCHghRYp2oLvn1NLfzDIFp8SQQPGGywHkzOrBCl1KHCS9puDKR53oNef8s+"
      "nUytFyYNUslSbMqW"};
  const auto signed_data = crypto::base64::Base64Decode(signed_data_base64);

  EXPECT_NO_THROW(verifier.Verify(
      {signed_data}, {crypto::CmsVerifier::Flags::kNoSignerCertVerify},
      crypto::CmsVerifier::InForm::kDer));
}

TEST(Crypto, SignatureRs1) {
  TestDsaSignature(crypto::weak::SignerRs1{rsa512_priv_key},
                   crypto::weak::VerifierRs1{rsa512_pub_key},
                   crypto::weak::VerifierRs1{rsa512_pub_key_invalid}, "test",
                   crypto::hash::Sha1("test"), crypto::hash::Sha256("test"));
}

TEST(Crypto, SignatureRs256) {
  TestDsaSignature(crypto::SignerRs256{rsa512_priv_key},
                   crypto::VerifierRs256{rsa512_pub_key},
                   crypto::VerifierRs256{rsa512_pub_key_invalid}, "test",
                   crypto::hash::Sha256("test"), crypto::hash::Sha512("test"));
}

TEST(Crypto, SignatureRs512) {
  TestDsaSignature(crypto::SignerRs512{rsa2048_priv_key},
                   crypto::VerifierRs512{rsa2048_pub_key},
                   crypto::VerifierRs512{rsa2048_pub_key_invalid}, "test",
                   crypto::hash::Sha512("test"), crypto::hash::Sha384("test"));
}

TEST(Crypto, SignatureEs256) {
  TestDsaSignature(crypto::SignerEs256{ecdsa256v1_priv_key},
                   crypto::VerifierEs256{ecdsa256v1_pub_key},
                   crypto::VerifierEs256{ecdsa256v1_pub_key_invalid}, "test",
                   crypto::hash::Sha256("test"), crypto::hash::Sha512("test"));
}

TEST(Crypto, SignatureEs512) {
  TestDsaSignature(crypto::SignerEs512{ecdsa521p1_priv_key},
                   crypto::VerifierEs512{ecdsa521p1_pub_key},
                   crypto::VerifierEs512{ecdsa521p1_pub_key_invalid}, "test",
                   crypto::hash::Sha512("test"), crypto::hash::Sha384("test"));
}

TEST(Crypto, SignaturePs1) {
  TestDsaSignature(crypto::weak::SignerPs1{rsa512_priv_key},
                   crypto::weak::VerifierPs1{rsa512_pub_key},
                   crypto::weak::VerifierPs1{rsa512_pub_key_invalid}, "test",
                   {}, {}, TestFlags::kSkipDigestOps);
}

TEST(Crypto, SignaturePs256) {
  TestDsaSignature(crypto::SignerPs256{rsa512_priv_key},
                   crypto::VerifierPs256{rsa512_pub_key},
                   crypto::VerifierPs256{rsa512_pub_key_invalid}, "test", {},
                   {}, TestFlags::kSkipDigestOps);
}

TEST(Crypto, SignaturePs512) {
  TestDsaSignature(crypto::SignerPs512{rsa2048_priv_key},
                   crypto::VerifierPs512{rsa2048_pub_key},
                   crypto::VerifierPs512{rsa2048_pub_key_invalid}, "test", {},
                   {}, TestFlags::kSkipDigestOps);
}

USERVER_NAMESPACE_END
