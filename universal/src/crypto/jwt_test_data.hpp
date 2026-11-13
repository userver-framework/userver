#pragma once

// Interoperability fixtures signed with Python cryptography, independently of userver.

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kHmacKey = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

constexpr std::string_view kRsPublicKey = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvxnBpAK1BQctUKls8qWq
jIjT07cf1sJUZ4kiaIbx+vZoC5vvz+GizyYsgiTDnGov7v32+fL9eDhZY+efeUzU
hfryBPQh9zncersgudUNVa3G0LqzVOgKAgnXNDFBAO8jV3eoNnE4QbtgvhzwbUJN
TJioP0cecVptINeCpqokynQ+PrO3FQQ37z8GVC1X+TzzRvfDxjPoqVQBHuG78+52
y7ctBLSZxQgoPB1kRZSyX3sCcSG6jwAFrDR+T2btGldzndpJeFKLiZlEnb5/9RAm
+U4Mys7l+jauHaysf3HIIdSrQ9lwAo1VGNJ856FDy1J1FZQSatWPUOCRDegeu3ji
gwIDAQAB
-----END PUBLIC KEY-----
)";

constexpr std::string_view kEs256PublicKey = R"(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEsw48zSttuBrxckPT7wKqzVCYxVvC
g4v71FexLoSkHu3pH1f+1ij9aS0gOjY0Nlz0BO25n9EPG+d94Pn+5nMuQg==
-----END PUBLIC KEY-----
)";

constexpr std::string_view kEs384PublicKey = R"(-----BEGIN PUBLIC KEY-----
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEpxmjL2nSmONhTsxDe5jT1uCgEnBuSaYI
dbWHUT9XQp0rJ+3f4FhfrJGhEdw7Xtn/++2szIw3o1B8HdSkiDuBl/ykF1VmoK+r
MAvtcADqNf5t5gHXA5FsKqySHufayY/3
-----END PUBLIC KEY-----
)";

constexpr std::string_view kEs512PublicKey = R"(-----BEGIN PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQAq9nK2h6jkXqVfe5bY8j/Oo5qzJr5
TBem9GkHrAiGHd5s/S35SWqGB4ngAGXp9qy6U+hHqIcRfp1h+y8N5vndryUAtS+K
3Jm9C/+i/V9MSQlsuQaSbAn3/kUb0lUAOq8T2nbltsNZxQE/xcEcp0T1G7vBg5OR
NYcMXssIoLhy9PeTAVs=
-----END PUBLIC KEY-----
)";

constexpr std::string_view kHs256Token =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.dlkneslo8bOguloMmymrLQcKKY-ZUe1TsjMmd2Rv33c";

constexpr std::string_view kHs384Token =
    "eyJhbGciOiJIUzM4NCIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.UJrwUAi2LHmGdPPvBYZKUGgxCxZqmEMgmgX4Nn2Z2cuy4YvirvJ3O3J7jkuVZIGb";

constexpr std::string_view kHs512Token =
    "eyJhbGciOiJIUzUxMiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.3KJtmnwuk0KQkt75nLzkyYaXXw2vBlis76I1RwVimsiGwDncagdZ_oC6b-LbbrBmPTFImriK93I-_EwP4yhbW"
    "g";

constexpr std::string_view kRs256Token =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.PHkSLIgJJlrWYODaRMXILQVdTe7BVbGUdF3_Vu63LsRQ78rzVvoUxmq_49Qw0q8AwAoWd4x0he9rPBlgZN33i"
    "PjeGVXU5VC6-nAZja_DNXhOGfwhdebxwFEUvORytzQoS5VBP-HbprKqll44vYJ6-gl47LAwDaBtggehiXVLm11oD4I85nbNGRn2C"
    "oMgJLHXNVUeOY42CttylmOLM3dobyhyGEczlEtAIid7gQJtcQA7StF2YaLQ90uBN9D_dK1b1TVcNTgj7NQRcQFXbjjs9VpFskOx3"
    "O-heKSlK7HdRmZREg9y5UlJQxxI0iydrL8NSZwHadQvfBF18l1mggERxw";

constexpr std::string_view kRs384Token =
    "eyJhbGciOiJSUzM4NCIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.k2yyBFM-hmKvf5bD_JV75m3dzrYLblaSMbzJpWfuxnUFEbneFERebBZ61YrDJ33w1o9-T0jXS265ca7TocKeU"
    "hhEg-3VuPlh8M3ivRw49LeG5F-0Fj5YGSRya7i3za2BCYza6q1S8VHE_p9qtJMWEY7toLvZUeIGRKSsPyxe5JYHiK5-oX3bVJueo"
    "eh3iGoWO5-8OI3i9yfKBz6kCmZWR5ME67jLOfswLOzrE__suT06sD3-nqGnDlxJ_CT47Tu4spOMoG-UA1qfYeEUiTbQjSYclmxhT"
    "h-YGvngfNa_miIWXhh4fXuskQkQL7Cj3ANaK7y8TdiyaPoXSKnp4-zP3A";

constexpr std::string_view kRs512Token =
    "eyJhbGciOiJSUzUxMiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.mxXfWkrjtR15Xd1EWNplGBGwfPU9Fn_XdIzSn7LMrcCvrxSVE_bx7AUHIY1HL5J0tcFUMgGPiOWWtRmFuYDzn"
    "ABLD_ajBuH7NxAN3WPtMEaSmwDtPXI_TzpG7GzWdRfAjvuKWOfrUQ360M9OkSTlo7X-pEUyqS5ja5iesAKkMORTSuoVJ9br0oOyd"
    "ocug5kdTRVLnrHz6m1doemqinaPrABBntcA5Y0nbCvQDcyp1DEbnnhSWErgRkqOnMB54cDCuncKGvl9wtkgci4Bmt-6tFO4BP1RQ"
    "pWh_iIgnoqHH4-RJ6Yy46POQgDhD6RLl85xka5cdnqQoIpjQZ88oh62hQ";

constexpr std::string_view kPs256Token =
    "eyJhbGciOiJQUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.G0TGerS7k-k5edZk0nIP9M3-A-YC60WldUPfN3rikqlouvz4mwX0vnYeORpF9C8NVfWYPkvR3jNmpYShyB_P1"
    "7rlVpXsFnCraW46pIxo6zi-Or6cRzM3H2W-1zITI6G7EjXulYelUe1zXnGI0lb-7aOq-EG9x_R1z-qTM2yHWY0AN3h_FxDux5L2g"
    "RtEAd6DEgXNz3grakzJvlgRYEMxooe5k6rut4pKWappGePQymGoFDlQGAUkQVJjheIImgQevf1rr02ygzDtS7PuZ-lOM-QjBGbfH"
    "phmkC84u1wuv_GhGoW6LKquT7F1Ya6rxngJkGChpMne1Lkxo_2nrgEpDA";

constexpr std::string_view kPs384Token =
    "eyJhbGciOiJQUzM4NCIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.lIDnuSJkL_24KCrpmVPJrzD6MCM2H645wPMnr94KEU9iIHYQotim7hR3lbMJ-8PeaOslQuixXl20pU3YgHfrA"
    "FDv4lGTeiuUl78zy0pKQ44aUyZassocoKxAmlU3M93H9vnr8k0GnjEG98EGdlRKE1eLj61WKqRj9h0rN2Zl-x_DIZkCZAmo24HGB"
    "lMxvpk-zUlXi-wTBhSw0Aw-_b35OG8jPTntaTDSevlZ-PDD3zdj9pjRie9u0zjuafIJ_9_HKU-PVOk7-9dxcBShFPf4NFWScfkIq"
    "LVIzvOcsV-gbZVlkXPJVBl83YY408KpoPYwVLX56mmrcvbbeAfB6vRqAw";

constexpr std::string_view kPs512Token =
    "eyJhbGciOiJQUzUxMiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.GKmovzrrftSB8Xcfoft4EgDS5i9T5f4hTR-_YjRD4GqCI_BzsS7VWlcQtL5hAYrwXm55zj6-OqSAh2V2jMBKm"
    "F6HrF6z5niakxLGdSO6yKEy7gZS8c3Y2hqoIIAgsrQmycUBRcNg9TNx3esNXgQbxWBUnGtZYlYXkYuhjYB26mg3Qy-puVLxSs045"
    "Yo_FhWnwHOOxsJegwy7-4Twad_aPndeuKZIbexmR3BzQKi3DZrhJOWJ5hOFthOGImYEcrMFM2OqLP-n5dRFDZcmU1jNxPIc1KthW"
    "EiPrrOdCCoveusYLcidinBj1ji1Zhwbud_PLj_2aX99_NLWabp1_mrjqA";

constexpr std::string_view kEs256Token =
    "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.bHWiXanLB5U1n8QUtT0O_VLcmBWk7t8a22OUlLEFbBw4nKf0NYKF1WiYWth9WvGBY6bf26riPdfbIrUrHp-d7"
    "A";

constexpr std::string_view kEs384Token =
    "eyJhbGciOiJFUzM4NCIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.I6yMCUx3hnmU7hrayzM4Mf7sPdORoLUejCOTc7ICNl-urDQGSUO7Blgj9sUsuGJhb29sH3Q5shAxlF7rs5n8M"
    "T9zxjZXIb5l7aoed5x8XWYwSDb6a4WqFF8BKxTVX3HJ";

constexpr std::string_view kEs512Token =
    "eyJhbGciOiJFUzUxMiIsInR5cCI6IkpXVCIsImtpZCI6InRlc3Qta2V5In0.eyJpc3MiOiJpc3N1ZXIiLCJhdWQiOlsibXktc2Vy"
    "dmljZSIsIm90aGVyLXNlcnZpY2UiXSwic3ViIjoidXNlci00MiIsImV4cCI6MjAwMCwibmJmIjo5MDAsImlhdCI6OTAwLCJzY29w"
    "ZSI6InJlYWQifQ.AGr0zju6T4GxDdrUCB7Hrwu8biXeVR0YYQmGI5jlqBIKpoOLcoDbZEEIfSJBzCRwSswVH5cqVghB7q_LJtzr4"
    "NimAdmxCmnRdJDkf7RbYjfgbf0kLaRFfkvSDH9KfDHDi4ug4lkO6j6Ed8GQm-NwrUQXCjdiQavg5RmheIPESKjTnuY_";

}  // namespace

USERVER_NAMESPACE_END
