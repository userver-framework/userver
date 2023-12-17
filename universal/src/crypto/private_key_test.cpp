#include <gtest/gtest.h>

#include <string_view>

#include <userver/crypto/exception.hpp>
#include <userver/crypto/private_key.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const std::string_view kEncPassword = "test_password_123";

constexpr const std::string_view kRsaKeyPem = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjTeIOQqFQ5WXe
3epAxyDFUN11yQFu4Bz6DmFkPjuNjBxMHVeZ2dTHpjFG5sSokQUG+OSwJ3M98jlX
yvgQbDONhzGvORH55e8I3+OuYQN/i7HcUUDhIWc/OmnyO2/DUI/E2/uLANCDC5cB
gjpeysxiAxaJZoRBK0Gq0VIubJbAVRrtgMgby2J0JLFYyPnPa3rIElJwNZEX5a2+
PnwOzeRqWTGTIUPVaxMrxu9Ti/1lGAUg9DCzbg36VALIbnnMhc1CINmGpNAXv611
M1tvbSrk8qpXW6HY7N/ezX/TW4UIfHpBXGuR1zSkFqGEJ/cgiOigdNtyjjwobIvR
rO40zhP5AgMBAAECggEABsCztk8/CG9T9RFMHH2fd0lOvw4exwzxZc2ubUy9fa7r
hKI5+xys1cyjgQEN5MKKaK1mF3qqI0ee42h7NYKj5xhOD6bWfXTqm31WluFgGagX
JLsfaa/N6ylpNCENEBCGwWcvUaIo/SI/jkpqS30rUmo6Rlg848z8HConsc+/tBg/
5uaDB8rNioblmLJjdFr5KcXkzwP210zmkh4cZehCnfOCw+by3LxJrSAWF/AlbI2k
X4apxCPzgXPhsbxw+1dcKxB4X5eWqUhPNtdlk6B8oVL6tfBvN/fxLUbbBRWU1lKF
NoNJCnK/G/D0YnBXyYJULbLywMu+oi8Yx9xEvKY8GQKBgQDUOI/iFflM+CA0weTY
RPaaTq+TWAXVSh8r4MeglN5EezpYc8WuuGkQH94J8TPvxZqBprNnS8pMnmS7vxNh
5Dv1iokX5Rzuixb1YMK99v28wY4yVkuwSf8xgCVsaSoPKkWdn8e2o12dvFCldk9M
gjJYgN7lUk359XWpu/uKIWrWQwKBgQDE/gSNw4WhWxozs2vEtLyG2dQ9BlqiXiRq
6Iv4AMUQ5Of9pDPVwdXDbr4VLl2XqiiYt15ToCgKJ0f3WCB1EgJ5Nk3wGhJXvL6W
h9F/JRx3OYi9cY5q0DBD4xfk8ZlhGEbGniIRiEiyzVEx9NbBfjWHnnCzFH2GmTNw
Qi5n9ODPEwKBgCBXznXia3AzkJT5x+q99+dkfpWyIJF1DnMdes8aYRWGwkmgu0v5
humSMcDKJeQw9W80/LqNbnNm4GtMn6OXqRuu1V3y6Qnh4MncyVEyR1FxHj7FsBtZ
666bnRh8npBZHOr84u8OzsGvZYsSENvUK+AZI99HP+MMabZIk/llRmcJAoGBAMHg
DrNHHxlzBBKmHwZ1qqY9dHiq7ECozsa5IChw+YJrBzfGh411O+Ef+Mv32f0OZu4G
ajt7gGydxGJWxXrywqRxIbuQrUTUae7UtQahi6Z7ZWytejD9vMLTmgylCwNYVS/d
KkJ/Eq2HDMZR5ZqBkEVtVhWpsPDrjoiJWgrOIwMpAoGAUXChd3jsmWUnaIH72Ck2
UXAbc4kKiiQw8NKaNahi6HxWYQOW4oOKdduCSKLgkQR+LR73lL+IO/NElWiKqQ85
M4LCsinjDYI6GMomAhKWwM8eUL1P+hAAIw8H4hZ2fcTPrkVOtyMKlfMJfj9u/fJY
+3HWC+alx6TPsKwtyvGWtFw=
-----END PRIVATE KEY-----
)";

constexpr const std::string_view kRsaEncKeyPem =
    R"(-----BEGIN ENCRYPTED PRIVATE KEY-----
MIIFLTBXBgkqhkiG9w0BBQ0wSjApBgkqhkiG9w0BBQwwHAQI/+VBHbHAI5UCAggA
MAwGCCqGSIb3DQIJBQAwHQYJYIZIAWUDBAEqBBBwgR+qRSCv8G1zlW/IIYS/BIIE
0MfbM7JoUerLB/t/xWjkxczy4AX1PaURvUwWNI5mmFF1O1ASkXs6fB32ggDvGqAw
ySGgsjjqLYt4adq6/m4+UeuhtKNTC0h+V/NYs1kwEQNXBMF49NuBJ330KwnltOZR
uibDwmj582HgBtI2oe5jzFTZfwF7NdZfuxS/yBqqfFEYtWYRVUB9w3o7RKKg/gYj
m5JYv1ZrcG/0JlQ9gibMcVzPGEfEs3XMn2WhjbNLsIheWYCTAfzXTtTDDnhjIx4D
HvVm2aYTeyReqeh/iZdxvjBDcQIS3R0GUFk8hFS/q+vWMnjS568js+9eQUWX8V/c
nX3pK8Cvos/74Ja53uOvZK8SYIJA/N2ViKMcF8JHoQKzo8IcyctjEGD+oe4xhXOV
p5DOvh1BDVDJBonDcZwqrDsAbcJc8Bn7xSOl/uW73LjrKFt2t7lXbVsA9M6HPibu
fFqidHA718SJO7OcvID/oT5EPc5NyGNIKfXAdlStlh/TNJNfIbjTpl/nhGPNrXy5
dEAKPP0/E8WhMbcKrLUgiwdD+fn/bczLsy8KeNdmwmVmizh4TlUBRHBum/jp0ijT
nGrOqwhCCd15m+FMvCdgHyojQwRpCZblEgNXawiGgApTnZAFUfuVm5rh+ihC/aIq
2fUOvTPSZMxmOeqyzoj32OBIPxhuXnwXtkXJyfaKY/LCQ2sIbcQRsZVnDDn6jcGZ
29gkxgfAiJVTqDgFLTqGLP4VJAEjsUn7Ld8I0r2Q5eWdIQTuKsL/I+f54WZ6iTdI
Sk/zsHnhOmPXTKqFx6dZPbjdrByfhEr8XT5+5g/fsw+1Rs8hfu+hGXGRfRNj6/g/
+McpLtPRkvWl6VUjiUTX2ANrW872KOFdWGT7TeJCHfhCF+OWTFl7mWpZoiBQY/E3
sCktcbiOI2XXJSajqPO/jwbSYKaTHxpwqbp+nnQ3vdyi5/1rrE6xUzOXirtnO7Qm
eucgtvAwxQYN6QLahNhng0oEBuKyfLjkgH8x/rVialV1UNndPVvbTw5kv06D4Re8
ixybVpucIyJK1Paz0QDuQpyCL9NEpellVpvfg5gerow+FlX0Urgh5bZVMSUtmJc5
E+9bPzin5gKafl+2anPXwCPRz2AdNOO3/UjeVqkyaqhoK7jxW2/YqLdmcFBDWlqk
w6RArP2BLr1DnxTEN+ainJ7Joq+7ANkcub6KyOwbOOfgpsFvpPf6+PQR+m03SHfE
FiuP3WmdQXrME2qczef4V36MdFS2R6LzWfu7Ad4A3cMht7mXtInbeTKRXxA2xpj4
5ZF6uhnN9912Pj7+rwthf3CNLXS6KH6UuCb/JADoXfu/wjzeKraXtC8vWNHfLcPr
SFTvRoOZx2BnY0dQcOPN3utfI1XLHNOP+C4WcHloMhR82d25scvmLbb8Z8XtE0L2
rUTtTAUCKyQOF4DxBnvxbwYnpteV9vQ7TQfB6+2GJffMoad172bzt5yO2zJ4XpB5
vnzJ7Nhtf04Tpk+nd/PE5+XyQnwQSRJDlYhTchw4BKGcKH1xb5fdQim8UGk/mN7a
tqqPpltH12U6nHKj/0ZbAUdT/wJK0q1ivShkF6XBBxBgCZqHauuPXspAndeYExnS
yDbe6W0pNw2H93N4xsaix2c+ZFrkjO4DwHF2iccmsDHQ
-----END ENCRYPTED PRIVATE KEY-----
)";

constexpr const std::string_view kEcKeyPem = R"(-----BEGIN PRIVATE KEY-----
MIG2AgEAMBAGByqGSM49AgEGBSuBBAAiBIGeMIGbAgEBBDDey/kD9F8ntWqDXIyP
3Kr0v/kJvccbdEZCnmDkNnUIO3HFG9MkgH04A4O2J9pJqUShZANiAAQS50yLyWGf
qOO6BWHWSss+h29BP2TOgcgcUy7pRy5/toMHU9u9Ku81WawfgFzKVVd4TIgL0gIj
ta3pGxg4DKXcNEF8QjImOCZiXe5xSc0+6VOJ082fMvaLrX0YcYUPvjk=
-----END PRIVATE KEY-----
)";

constexpr const std::string_view kEcEncKeyPem =
    R"(-----BEGIN ENCRYPTED PRIVATE KEY-----
MIIBHDBXBgkqhkiG9w0BBQ0wSjApBgkqhkiG9w0BBQwwHAQI1Wwdl9uUs0oCAggA
MAwGCCqGSIb3DQIJBQAwHQYJYIZIAWUDBAEqBBBSgWxMLdK4Cf+oAjkyTNl7BIHA
7m/GqWqSTx8lqxbOyqOveFrC18gcPovG2c7ggt7C0wg1vjxyaj4SNQzLr28W94k8
3/d+6hHnDD6FQhujcL1yscgZ1PfAFkl6+kjdeKJg9DdZ81rhiLeaL2jgMPdSG6Cl
3AR47lgqLcQa1DwqDHfrhgsXD4y3vtOETTGa99Bj6K720XNJYbHxUrsBKhNWi6fP
Axwrwe8h4BQ7K1dHMddzuJZwIUAkwn1+IQO8XIAO27UPd9Jc2+jPsDxl5rc0dw9E
-----END ENCRYPTED PRIVATE KEY-----
)";

}  // namespace

TEST(Crypto, PrivateKeyPemDefault) {
  EXPECT_FALSE(crypto::PrivateKey{}.GetPemString("test").has_value());
  EXPECT_FALSE(crypto::PrivateKey{}.GetPemStringUnencrypted().has_value());
}

TEST(Crypto, PrivateKeyPemEmptyPassword) {
  EXPECT_THROW(crypto::PrivateKey{}.GetPemString({}),
               crypto::SerializationError);
}

TEST(Crypto, PrivateKeyPemRoundtripRsa) {
  const auto rsa_key = crypto::PrivateKey::LoadFromString(kRsaKeyPem);
  EXPECT_EQ(rsa_key.GetPemStringUnencrypted(), kRsaKeyPem);
}

TEST(Crypto, PrivateKeyPemRoundtripEc) {
  const auto ec_key = crypto::PrivateKey::LoadFromString(kEcKeyPem);
  EXPECT_EQ(ec_key.GetPemStringUnencrypted(), kEcKeyPem);
}

TEST(Crypto, PrivateKeyPemDecryptRsa) {
  const auto rsa_enc_key =
      crypto::PrivateKey::LoadFromString(kRsaEncKeyPem, kEncPassword);
  EXPECT_EQ(rsa_enc_key.GetPemStringUnencrypted(), kRsaKeyPem);
}

TEST(Crypto, PrivateKeyPemDecryptEc) {
  const auto ec_enc_key =
      crypto::PrivateKey::LoadFromString(kEcEncKeyPem, kEncPassword);
  EXPECT_EQ(ec_enc_key.GetPemStringUnencrypted(), kEcKeyPem);
}

TEST(Crypto, PrivateKeyPemEncRoundtripRsa) {
  const auto rsa_key = crypto::PrivateKey::LoadFromString(kRsaKeyPem);
  const auto rsa_enc_pem1 = rsa_key.GetPemString(kEncPassword);
  const auto rsa_enc_pem2 = rsa_key.GetPemString(kEncPassword);
  EXPECT_TRUE(rsa_enc_pem1.has_value());
  EXPECT_TRUE(rsa_enc_pem2.has_value());
  EXPECT_NE(rsa_enc_pem1, kRsaEncKeyPem);  // should use different parameters
  EXPECT_NE(rsa_enc_pem1, rsa_enc_pem2);   // every time
  const auto rsa_enc_key1 =
      crypto::PrivateKey::LoadFromString(*rsa_enc_pem1, kEncPassword);
  const auto rsa_enc_key2 =
      crypto::PrivateKey::LoadFromString(*rsa_enc_pem2, kEncPassword);
  EXPECT_EQ(rsa_enc_key1.GetPemStringUnencrypted(), kRsaKeyPem);
  EXPECT_EQ(rsa_enc_key1.GetPemStringUnencrypted(),
            rsa_enc_key2.GetPemStringUnencrypted());
}

TEST(Crypto, PrivateKeyPemEncRoundtripEc) {
  const auto ec_key = crypto::PrivateKey::LoadFromString(kEcKeyPem);
  const auto ec_enc_pem1 = ec_key.GetPemString(kEncPassword);
  const auto ec_enc_pem2 = ec_key.GetPemString(kEncPassword);
  EXPECT_TRUE(ec_enc_pem1.has_value());
  EXPECT_TRUE(ec_enc_pem2.has_value());
  EXPECT_NE(ec_enc_pem1, kEcEncKeyPem);  // should use different parameters
  EXPECT_NE(ec_enc_pem1, ec_enc_pem2);   // every time
  const auto ec_enc_key1 =
      crypto::PrivateKey::LoadFromString(*ec_enc_pem1, kEncPassword);
  const auto ec_enc_key2 =
      crypto::PrivateKey::LoadFromString(*ec_enc_pem2, kEncPassword);
  EXPECT_EQ(ec_enc_key1.GetPemStringUnencrypted(), kEcKeyPem);
  EXPECT_EQ(ec_enc_key1.GetPemStringUnencrypted(),
            ec_enc_key2.GetPemStringUnencrypted());
}

USERVER_NAMESPACE_END
