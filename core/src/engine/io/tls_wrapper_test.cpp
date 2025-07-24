#include <userver/utest/utest.hpp>

#include <openssl/opensslv.h>
#include <sys/socket.h>

#include <stdexcept>
#include <string_view>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = internal::net::TcpListener;

// Certificates for testing were generated via the following command:
// openssl req -x509 -sha256 -nodes -newkey rsa:2048
//    -days 3650 -subj '/CN=tlswrapper_test'
//    -keyout testing_priv.key -out testing_cert.crt
//
// NOTE: Ubuntu 20.04 requires RSA of at least 2048 length

constexpr auto key = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC/yvHZDWFPnDJQ
/1NCDvmj2n3osGSv9+Hl1Caguv5IHSwn8XihcGiPHmm6NFEMU1dZjgUVaZ39nIka
adUI+7dP6SKOYygjtJnJpCNDRKdvO3Axz31gO9DUnOdu7unn4q1ydIYM6To/cwWz
shYFxoGEGwJzIKp93aRX0qMbz3HlUMVlo9tgwflmU4Qm1nYktJVLbsGsUcVPMB3J
+HJq7ciGFs70JH2O6OAISPdSy0+kUhv2u8zN8fRMBMRxn56ZrW66Vyj1C57uJxoW
9U8oEx+U27/YHY/BhmtkViIF+2snH6/cYv4mBliSfWeihBgVhG/IgCI48z7leWLv
0/zFmOtzAgMBAAECggEADU9EIU/wZNnuE/jkCj2HzXsoKbG0CxIktxJV6+mOI+sC
WXNEb8+hMe1mYOmohjZyZWCZsba2pBbs3MxjYFA3lHAVWdQ/wNqToY6mc9Cb3fg9
/PbtOHRuNZL97JDf4pu0dbDobJTy2dxdlO7S4Gu6KTTGor6tljZ/ZSjU8OUgfk1R
wPma33f7z4M03ZzcM0u3i5D+4oD39hFMrNQITk8lv8u0Ha+C8VP6Sov8UKOQ4G2m
kyUs3SGLlmHOIkaIZ2+AfhLvk5b/WgreprObKYJww0PSzFjaIBIdfMBVNieI8aqW
1Qn2FDTras+afjJTTL9exrlP40XJqjvCQtd2O6UtYQKBgQDzo5eDUjGjBo/amOTg
gcgj2yH/sUs0j4Rs5rJ2oqFT45HrGP2JfhMUSlw4uyDFBbaoMzpYOr9S/a+zu+rs
fAZboXdkoD6HcsG29tfsVK+3ZWKlgLm4T2fP7PlxH7wyVqYIqjRKLIXxGYzFjOZO
y4SEoCk0KfxbXQZ5hW1YyoMHRQKBgQDJhffdD/qmjqbSQt7LcH5oeQzmtXYaXzvc
OQRmX6ICxDXdst7eZ1riHXyukkTpaODbEIW7mAP7mMejuxJvImLbZTvDEDWfapwP
gUEUonCQKrY6h14S1Z/CCAiZHkStJrxFRmbQ5b6RXcBgdpyTh4qj5BzrGTooNClC
NH3+/kVXVwKBgCToe2NhaDOSIuiykLmR74e/An+BlCr6Ms1shUyDhnz21HwQ5ReX
CbzhJudRMb2nB+yjFguXmrQvyhYoOYZpo2zuIPAVdmN+duoIqt0aVyQpL7Byt6+8
F7Xf6EnCzPezOKPHZPR3mjLT9AdZOOpm2kRdHuDQG3KbvQdbtxzkUMUhAoGBALNb
IHcHObXzUFXiXhgCTv78fZb3+d0O1V/y/w9+HdsIdkiSYfjfU+vbApT8aYizZyyR
T/TeHu1V1JjMbmOq3wEU4FODobX4VF0YVKvgxv4IhZch04A/0KgILl7YqZbR2s5t
EiTp1Onb3tP7vO8wuxuScoprMW+GvRHHVjwUYfKRAoGAST1MbnT/Wc3HIkI6qOiP
iFqpu4U7AJm9ym/Zc61auoSBKhkWRIcf0sIaQbkH3gOFUKKGmvZ8D2fOn1fco2z+
QXz1smGbGr3A2/scVwRYLef1F/XUHPr85vu0REEtLpizBNGD1D4iViAJk54yRGb2
sWkERQknCgvHIs2c40/YZOE=
-----END PRIVATE KEY-----)";

constexpr auto cert = R"(-----BEGIN CERTIFICATE-----
MIIDFTCCAf2gAwIBAgIUPx6k27O6Tf4me1RyBQ/FWhsn364wDQYJKoZIhvcNAQEL
BQAwGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MB4XDTIyMDIxNzE3NTMwN1oX
DTMyMDIxNTE3NTMwN1owGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MIIBIjAN
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAv8rx2Q1hT5wyUP9TQg75o9p96LBk
r/fh5dQmoLr+SB0sJ/F4oXBojx5pujRRDFNXWY4FFWmd/ZyJGmnVCPu3T+kijmMo
I7SZyaQjQ0SnbztwMc99YDvQ1Jznbu7p5+KtcnSGDOk6P3MFs7IWBcaBhBsCcyCq
fd2kV9KjG89x5VDFZaPbYMH5ZlOEJtZ2JLSVS27BrFHFTzAdyfhyau3IhhbO9CR9
jujgCEj3UstPpFIb9rvMzfH0TATEcZ+ema1uulco9Que7icaFvVPKBMflNu/2B2P
wYZrZFYiBftrJx+v3GL+JgZYkn1nooQYFYRvyIAiOPM+5Xli79P8xZjrcwIDAQAB
o1MwUTAdBgNVHQ4EFgQUvl4L7fVG2b4TxPbhBDvCT7BT924wHwYDVR0jBBgwFoAU
vl4L7fVG2b4TxPbhBDvCT7BT924wDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAQEABWB1D1XgMdBabNxxmGq7Y2iBxsonHjuVlASc8InSJHr/jFHjHvS9
y2gtBkQ9P28lIJROkrzXiYM5idyxdijOvwQLKTcJcH95N/h+LPRGvy8a2j2c01LZ
xfDPBOI2KLymnwJS+kQJSm6l5BJBaZxtfnLlgzTaDY1WOOSWmpw0YFS6rbLDrpU3
2Ue8SVlHRBxIID/RgIFcuASSCW+1gvIYDIfvZ9AcS29QPl1oDBoPXAzL8rWOChov
VfX6aXy77rZ1efGfLaWZD1yADp+bg033FkqpOf4PtZpixzbZyDmbh6hrvTuX/y4w
VsNtFCX7LLH/W4mSvkvIws1tm8OtphLn3A==
-----END CERTIFICATE-----)";

// Certificates for testing were generated via the following command:
// openssl req -x509 -sha256 -nodes -newkey rsa:2048
//    -days 3650 -subj '/CN=tlswrapper_test_other'
//    -keyout testing_priv.key -out testing_cert.crt
//
// NOTE: Ubuntu 20.04 requires RSA of at least 2048 length
constexpr auto other_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDRXTl/iZajFxZa
uJpE3yykbFE/o2TYnPQX3LclfToXv6RQIAPjGvwrvBgH4m1I9PaYYpnHolnxEYDN
ur0VJutfkypdXKK9ONcVIx76N98z2hBA3u6XLSmorXu8wsHzgSBioEB1bjEYkvo3
tZooStDgvR/ghCuOXJI4K8qjdUd+T4VhWUeP6qS3KlAOxVqwtr2U5aN3gpJGlZQu
kJykUjQkQp1jcoW13EmYFnE2pRj3Ne3+TLDTVeM2smQf5dwL/eaesU6Yc+Mi8Q8G
tMxH5hzT14b/xG3Fc5nGPUK4Kt8Il/+Nry90UGyFVhnbLcel4i1Sh8QXLZZPeCGM
aJSCnB+7AgMBAAECggEBAJlXLkW7ABlzT2wiyNqomonSy69QfQwp6J2RipJqpaG/
Oxl0WWR83zUpDnC35lMJF5OEpB0TS8zEhRIpM1PKrZnSr7SxpH/yoZVZo9agFVpk
3IKmxRj0ew6QAZC/FE7ExHN3674Wdt8IxzsGR2I7acEww6gtJbmfE3kQmdoei752
LGQbRkNM+DTYYk2yZeCXUVkOqA2J/d4OAPnQyjAhkBRcKU71jF1Zc/IZTWBfgn5e
s4MX2T1KTnnawCBDWMW6oJS8gRRmRGwPkEFVRJ4K4V/kgEhn5wT2mokF+hHYMdo+
V41bqiquvl9j0SqhBnD6OAYQi32npCCf4ddRhlUZbAECgYEA79IhYYiX8uc5Ypnq
lNoR9VN+MAEyECnWrwNlf3obyVj68/KGusjFbljSy/rBSS6ShPQCeSRbzPS3j1B3
nbqZZy+ZUbviAbOpw2D4xN+ur7jVOxxhLjEuTP0lnuPJCFflMQtN5vfkhdcDG89h
YSRdm9MFcPXSysDHzoXgG1KXyB0CgYEA330WOM895a9tDKfxtVppznuZ9mJAEDwV
xQqloZjpnbTbeL3l2CMo2tgcZRjVoYtc1GFE8ke0ZGafujZ+qiGAn8dYCsQgFY+Z
PDKBSbGFQNhq6gcDbA33/Zkh3QvsDxUif5lRSSKEmXbYiTuoo9mUDQ8qKsb4T5v6
uMovOi0d77cCgYEAjlnncJJ4xzkS6gE8qhBrOnjN3UbIZanAAfB9LdbYaYLEq0rZ
SEPmVSKqNWPpmTvowrxoP2oih5z23D3CUsCxT/uEAW0JsULo0M1dvNadRTbscwLc
eGO+/PoCe7bv3GD37U2tdxzL69n9wWMuhU/ltJnkj/GKpskZkPAMX4t+Bs0CgYAf
ln+AkhIul6fzJP2t41SXIbM2NtbVNJjjG8kjWQiUCM8Idta4wOdyXx9MTsFLLvZ0
8jabg/UER9kFqdQnWcrjSnqwMt5SDdTbxEuvzc6Gxs/9ufYK3MKTboRxyNCZpSQW
IuZxTtatFjYu12bTmdoqKl2MZEkOf35lhfY848maawKBgACxJXDPWx/Ah4cT0LbN
XFdWxjSVMU2BHlO0rWy8oZwFaOH8cy+dYe1cdMh+nQLMNXXTY+nu4WditpLfURof
2GmF9LD5e6tUJw1rPLbKDVM5DXuO3ka0msRea4Y8Diaiql8eLytJhbMw12nE0B1g
LKaaQekBQ5eebwM3oRc68q0K
-----END PRIVATE KEY-----)";

constexpr auto other_cert = R"(-----BEGIN CERTIFICATE-----
MIIDITCCAgmgAwIBAgIUL5c8L0jabF9rZ2w383zGQmwcLZwwDQYJKoZIhvcNAQEL
BQAwIDEeMBwGA1UEAwwVdGxzd3JhcHBlcl90ZXN0X290aGVyMB4XDTIyMDIxNzEy
MjcxOVoXDTMyMDIxNTEyMjcxOVowIDEeMBwGA1UEAwwVdGxzd3JhcHBlcl90ZXN0
X290aGVyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0V05f4mWoxcW
WriaRN8spGxRP6Nk2Jz0F9y3JX06F7+kUCAD4xr8K7wYB+JtSPT2mGKZx6JZ8RGA
zbq9FSbrX5MqXVyivTjXFSMe+jffM9oQQN7uly0pqK17vMLB84EgYqBAdW4xGJL6
N7WaKErQ4L0f4IQrjlySOCvKo3VHfk+FYVlHj+qktypQDsVasLa9lOWjd4KSRpWU
LpCcpFI0JEKdY3KFtdxJmBZxNqUY9zXt/kyw01XjNrJkH+XcC/3mnrFOmHPjIvEP
BrTMR+Yc09eG/8RtxXOZxj1CuCrfCJf/ja8vdFBshVYZ2y3HpeItUofEFy2WT3gh
jGiUgpwfuwIDAQABo1MwUTAdBgNVHQ4EFgQU947UQygfsOw+udnjR3rywSlqnZIw
HwYDVR0jBBgwFoAU947UQygfsOw+udnjR3rywSlqnZIwDwYDVR0TAQH/BAUwAwEB
/zANBgkqhkiG9w0BAQsFAAOCAQEAZUrhuONUlLalvmtjaVEsNgoPErF8jPeLHHGA
9dVAvkaumbwacO5AYWbODRhC0NCdYTNAqLaQVA4utxZGI3Z2pQKxI+OnlCM+ws3z
Nrfh+E/6EJZblfqK7SBduPYlFxhkalQ19mi/R5E6p9EmBdV1sScR6Hsg3qSCpFbT
Z49kghXJ5KAS4jOB6SxClxbR5Tpc1E3khduX6aGau1VOkgPJxfdqHHqsyUc1RH/Z
3W6n4SmhCZxEpQzSQUw4YPIIpWuSUWUr7MS7TzGDdT4AgD0m5VLFbF2ca/Fsz2WW
v2R63aFo/UfQSQ4dhC0o2Vy74DyLwnO3pH8wudfBJ8/LX/Uz/A==
-----END CERTIFICATE-----)";

// Certificates for testing were generated via the following command:
// openssl req -x509 -sha256 -nodes -newkey rsa:2048
//    -days 3650 -subj '/CN=tlswrapper_test_other'
//    -keyout testing_priv.key -out testing_cert.crt
//
// intermediate certificate was generated by
// openssl x509 -req -in <(openssl req -new -key testing_priv.key -subj "/CN=intermediate_test") -CA testing_cert.crt
// -CAkey testing_priv.key -CAcreateserial -out intermediate_cert.crt -days 1825 -sha256 full chain generated by cat
// intermediate_cert.crt testing_cert.crt > fullchain.crt NOTE: Ubuntu 20.04 requires RSA of at least 2048 length

constexpr auto chain_private_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCtQd5O3Zow01ZC
vbuUv9s7jm48KwSmVje+k887w/h5fNR+lah44owNw6P/7RQ4uMmgGPxtIxdkZNvI
p5TjBcqMWK3OP4ReG3c/Ak+A1b6NOC5KborngfO0K9uzV/9Pa53UfcWoqSyqcc91
G7zzDGuYHOs3FKk53fTNaRUYd5UMX8zZkTdTPpOJWDWZHIOYr6sgt7Py+NtVp70D
zS4/qIHD68oTjTYwp8DCQ3qljoTvSoYudKC+Xnm9lXnc2INJXmxYUFZBkq5vMczY
0mNFw4PNyEQyfyEOseaPFrAF/AWIiQWmYfe6M4gitc9Ct4cu7+VdLqDN3/D2bsbG
ceKzCbZzAgMBAAECggEAO56m+UyYeqS+0kin/A/pSR1CIcJL31Fb7WC/tzlAj828
8bJePvr2ZuYj0TWr97je6RCwDH4+1nU+jFXejiC4CoOZi5ef3SJmbnBFG3hyEfZ7
N3HCqte1HRLaj2SAnrvRnAWLtvZAQIbZdNsOsjRb8gRBjLq3YQpX6zd14u2DhLYB
vQxVfONtOGeUx/UUiss2MpJfrqoUoGVb+/onG1yAEbtWOs0Ig/dljopvcgkeCz9J
mB2MD5yv5a7sLBOcgyTpvE6ODgXzs+ZtS3TybGS+ld2rFrDlcInY1zXSP5iaW6Z5
lSUeolx5w1lVjRcc9WdybEOYpkV76EqX4v9XbGIxAQKBgQDUUX36l3YQDkBgZOvb
FnQaGUUAUiRenJedW5FMKWxBh2tXNUIa3aw7GtPhf4/8JLCaFrYF4WLkDfb0CfSY
wbunEjqa1Fa/CFmuagBHg6ClIiGX2/p9t2iQEEMZA+qjeAeFgfYy86e4t1oRn6iA
EED7j/7szqZvWrHtcv4Z9HP0YwKBgQDQ5xY0i+ouWJ7nGWEZ2A2arixL6KSAPIlm
NAPoYZGTWYXl259vCrJmnqCRpqtCLkCwhtFbDewPtf1PTWYLJUUWcjdljpuKfPpu
sIseA3CSRIfggW1X3A/txtscWZQhctie+jU9BFqiSaG/Mto+GdH4BFT4oUFKmfA/
J2rEKVyqsQKBgDoTCEhxAWQm4cj8Ed9dZuh0nQEXdsdCQd5S241fjzLlXaD++lPq
6l9IWUhG4hVv27ZqG+PD4I7Mmw3pYzQdWby7KbiL+CZMnGsup2DoShqhGVs2Wm/k
qP8u04uWHKoV/Mix4avSJcBKtqI3b5mH2J52pp4TcEbpId33JDXpPYZNAoGBAMif
5jd41+LCwXj4asTDNe2DsI8GUlXFzb8V3VrjuUdmBq4GCkw+Xa8oUNUQ2BCrEv11
vMJR0JAWG7x5fLLfjEZOUt154+9Qr8J2UmT0sLwIjOYT5ssmUTXucKf9b8Hf5iJn
8ZE0CUcqp+hUEjzp1zj2EBTn6SiYRp6gYG0bvB9BAoGBANLAq1ZJw7JsZ6rCvduA
GgdtFZePPol1/iQX7YXgP0jP1i3gqRKa7M621u8Hunl5BAqQ/1fu/7qAExzDcmkn
OmJhJDUFdNcQ6+4LCpj5JVed/J1Tnh8vUaRbYV+9l0y5WolcuAY+hfJuB4K0mUXL
11/L+DCO5OYM9ud/kl+ttDWj
-----END PRIVATE KEY-----)";

constexpr auto cert_chain = R"(-----BEGIN CERTIFICATE-----
MIIC5TCCAc2gAwIBAgIUEAEQSbPfQymprdTsER15SJz+eaowDQYJKoZIhvcNAQEL
BQAwGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MB4XDTI1MDEwNDExMjgzMFoX
DTMwMDEwMzExMjgzMFowHDEaMBgGA1UEAwwRaW50ZXJtZWRpYXRlX3Rlc3QwggEi
MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCtQd5O3Zow01ZCvbuUv9s7jm48
KwSmVje+k887w/h5fNR+lah44owNw6P/7RQ4uMmgGPxtIxdkZNvIp5TjBcqMWK3O
P4ReG3c/Ak+A1b6NOC5KborngfO0K9uzV/9Pa53UfcWoqSyqcc91G7zzDGuYHOs3
FKk53fTNaRUYd5UMX8zZkTdTPpOJWDWZHIOYr6sgt7Py+NtVp70DzS4/qIHD68oT
jTYwp8DCQ3qljoTvSoYudKC+Xnm9lXnc2INJXmxYUFZBkq5vMczY0mNFw4PNyEQy
fyEOseaPFrAF/AWIiQWmYfe6M4gitc9Ct4cu7+VdLqDN3/D2bsbGceKzCbZzAgMB
AAGjITAfMB0GA1UdDgQWBBQBq2+OTbGp4TvtWVkYe4LfWybT+jANBgkqhkiG9w0B
AQsFAAOCAQEAYaebsazdrABtiJF+FmLwmX9RJkLBxf4cxuSHzhieNeOSyVecCAmF
eJ1/4VKSWpDKfIk6pG8CWFO3EUCkytCof/idlbTqoYZ/WckzsYGrzxPTGSLw1Oen
/vnDFEE6cnIqJEB6v3LzfQrLBXr57+I7/SvpSGin9DuxfTQplNAt8lxDuiwJZz0+
IrzRwLB9HzXo8xvG9v88L35SW8SwWds6R5hjpZ928qOP5JbosI6ccQ6fgu8hSy5D
H9ZIE4FyGrLTbA1reSjT43yBYBzX0H4JsQnzGjbraGk5HYLFETVKbCYurYo+4Rl5
sYDUeR5wwfcKZCH43JERoG4DVI7yIKIzZQ==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDFTCCAf2gAwIBAgIUXOIkjRBGF8HhB43My6tlrFcOoJcwDQYJKoZIhvcNAQEL
BQAwGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MB4XDTI1MDEwNDExMTMxMVoX
DTM1MDEwMjExMTMxMVowGjEYMBYGA1UEAwwPdGxzd3JhcHBlcl90ZXN0MIIBIjAN
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArUHeTt2aMNNWQr27lL/bO45uPCsE
plY3vpPPO8P4eXzUfpWoeOKMDcOj/+0UOLjJoBj8bSMXZGTbyKeU4wXKjFitzj+E
Xht3PwJPgNW+jTguSm6K54HztCvbs1f/T2ud1H3FqKksqnHPdRu88wxrmBzrNxSp
Od30zWkVGHeVDF/M2ZE3Uz6TiVg1mRyDmK+rILez8vjbVae9A80uP6iBw+vKE402
MKfAwkN6pY6E70qGLnSgvl55vZV53NiDSV5sWFBWQZKubzHM2NJjRcODzchEMn8h
DrHmjxawBfwFiIkFpmH3ujOIIrXPQreHLu/lXS6gzd/w9m7GxnHiswm2cwIDAQAB
o1MwUTAdBgNVHQ4EFgQUAatvjk2xqeE77VlZGHuC31sm0/owHwYDVR0jBBgwFoAU
Aatvjk2xqeE77VlZGHuC31sm0/owDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAQEAOs+WfsO7k2zZS3CMAOHwxZzXy9PvdCzZFzJfjkKRh22zjZ1dmf8f
dlMGPqAENae3zuUd4ErFztNg6bGKW66j5JCQvfdqrj2XUUpTuJJWVcsvtxamDFz8
2h2+ky35xop4GLv3E9BPZvXSZ4wwiFbehI79tPwz3Qn4rsaowxM+bhMIgf+Wt4hS
nyLZGLR3MYH5wY6kCErZobxlXukvxS/+RTobvyo/9wbKjx1qaOXfuMKsFK3hpYzq
uwddqXEoNvE0VFCib7dsBnBUEeKhnVioD0vpDfvrCLNTm8VOg3xVz902WNnpgSdu
ud5lWYZNZ6ygIOvwFz1VST30jT4Lj3Bmrg==
-----END CERTIFICATE-----)";
constexpr auto kShortTimeout = std::chrono::milliseconds{10};

}  // namespace

UTEST(TlsWrapper, InitListSmall) {
    const std::string kStringA(16, 'a');
    const std::string kStringB(32, 'b');
    const std::string kStringC(16, 'c');
    const std::string kStringD(64, 'd');
    const engine::io::IoData kDataA{kStringA.data(), kStringA.size()};
    const engine::io::IoData kDataB{kStringB.data(), kStringB.size()};
    const engine::io::IoData kDataC{kStringC.data(), kStringC.size()};
    const engine::io::IoData kDataD{kStringD.data(), kStringD.size()};
    const auto deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(deadline);

    auto server_task = utils::Async(
        "tls-server",
        [deadline, kDataA, kDataB, kDataC, kDataD](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert_chain),
                crypto::PrivateKey::LoadFromString(chain_private_key),
                deadline
            );
            if (tls_server.WriteAll({kDataA, kDataB, kDataC, kDataD}, deadline) !=
                kDataA.len + kDataB.len + kDataC.len + kDataD.len) {
                throw std::runtime_error("Couldn't send data");
            }
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);
    std::vector<char> buffer(kDataA.len + kDataB.len + kDataC.len + kDataD.len);
    const auto bytes_rcvd = tls_client.RecvAll(buffer.data(), buffer.size(), deadline);

    server_task.Get();
    const std::string result(buffer.data(), bytes_rcvd);
    EXPECT_EQ(result, kStringA + kStringB + kStringC + kStringD);
}

UTEST(TlsWrapper, InitListLarge) {
    const std::string kStringA(2'048, 'a');
    const std::string kStringB(2'048, 'b');
    const std::string kStringC(4'096, 'c');
    const std::string kStringD(8'192, 'd');
    const engine::io::IoData kDataA{kStringA.data(), kStringA.size()};
    const engine::io::IoData kDataB{kStringB.data(), kStringB.size()};
    const engine::io::IoData kDataC{kStringC.data(), kStringC.size()};
    const engine::io::IoData kDataD{kStringD.data(), kStringD.size()};
    const auto deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(deadline);

    auto server_task = utils::Async(
        "tls-server",
        [deadline, kDataA, kDataB, kDataC, kDataD](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert),
                crypto::PrivateKey::LoadFromString(key),
                deadline
            );
            if (tls_server.WriteAll({kDataA, kDataB, kDataC, kDataD}, deadline) !=
                kDataA.len + kDataB.len + kDataC.len + kDataD.len) {
                throw std::runtime_error("Couldn't send data");
            }
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);
    std::vector<char> buffer(kDataA.len + kDataB.len + kDataC.len + kDataD.len);
    const auto bytes_rcvd = tls_client.RecvAll(buffer.data(), buffer.size(), deadline);

    server_task.Get();
    const std::string result(buffer.data(), bytes_rcvd);
    EXPECT_EQ(result, kStringA + kStringB + kStringC + kStringD);
}

UTEST(TlsWrapper, InitListSmallThenLarge) {
    const std::string kStringSmall(512, 'a');
    const std::string kStringLarge(32'768, 'b');
    const engine::io::IoData kDataSmall{kStringSmall.data(), kStringSmall.size()};
    const engine::io::IoData kDataLarge{kStringLarge.data(), kStringLarge.size()};
    const auto deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(deadline);

    auto server_task = utils::Async(
        "tls-server",
        [deadline, kDataSmall, kDataLarge](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert),
                crypto::PrivateKey::LoadFromString(key),
                deadline
            );
            if (tls_server.WriteAll({kDataSmall, kDataSmall, kDataSmall, kDataSmall, kDataLarge}, deadline) !=
                kDataSmall.len * 4 + kDataLarge.len) {
                throw std::runtime_error("Couldn't send data");
            }
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);
    std::vector<char> buffer(kDataSmall.len * 4 + kDataLarge.len);
    auto bytes_rcvd = tls_client.RecvAll(buffer.data(), buffer.size(), deadline);

    server_task.Get();
    const std::string result(buffer.data(), bytes_rcvd);
    EXPECT_EQ(result, kStringSmall + kStringSmall + kStringSmall + kStringSmall + kStringLarge);
}

UTEST_MT(TlsWrapper, Smoke, 2) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            try {
                auto tls_server = io::TlsWrapper::StartTlsServer(
                    std::forward<decltype(server)>(server),
                    crypto::LoadCertificatesChainFromString(cert),
                    crypto::PrivateKey::LoadFromString(key),
                    test_deadline
                );
                EXPECT_EQ(1, tls_server.SendAll("1", 1, test_deadline));
                char c = 0;
                EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
                EXPECT_EQ('2', c);

                auto raw_server = tls_server.StopTls(test_deadline);
                EXPECT_EQ(1, raw_server.SendAll("3", 1, test_deadline));
                EXPECT_EQ(1, raw_server.RecvSome(&c, 1, test_deadline));
                EXPECT_EQ('4', c);
            } catch (const std::exception& e) {
                LOG_ERROR() << e;
                FAIL() << e.what();
            }
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
    char c = 0;
    EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('1', c);
    EXPECT_EQ(1, tls_client.SendAll("2", 1, test_deadline));
    auto raw_client = tls_client.StopTls(test_deadline);
    EXPECT_EQ(1, raw_client.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('3', c);
    EXPECT_EQ(1, raw_client.SendAll("4", 1, test_deadline));

    server_task.Get();
}

UTEST_MT(TlsWrapper, DocTest, 2) {
    static constexpr std::string_view kData = "hello world";
    const auto deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    /// [TLS wrapper usage]
    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(deadline);

    auto server_task = utils::Async(
        "tls-server",
        [deadline](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert),
                crypto::PrivateKey::LoadFromString(key),
                deadline
            );
            if (tls_server.SendAll(kData.data(), kData.size(), deadline) != kData.size()) {
                throw std::runtime_error("Couldn't send data");
            }
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, deadline);
    std::vector<char> buffer(kData.size());
    const auto bytes_rcvd = tls_client.RecvAll(buffer.data(), buffer.size(), deadline);
    /// [TLS wrapper usage]

    server_task.Get();
    const std::string_view result(buffer.data(), bytes_rcvd);
    EXPECT_EQ(result, kData.substr(0, result.size()));
}

UTEST(TlsWrapper, Move) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            try {
                auto tls_server = io::TlsWrapper::StartTlsServer(
                    std::forward<decltype(server)>(server),
                    crypto::LoadCertificatesChainFromString(cert),
                    crypto::PrivateKey::LoadFromString(key),
                    test_deadline
                );
                engine::AsyncNoSpan(
                    [test_deadline](auto&& tls_server) {
                        EXPECT_EQ(1, tls_server.SendAll("1", 1, test_deadline));
                        char c = 0;
                        EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
                        EXPECT_EQ('2', c);
                    },
                    std::move(tls_server)
                )
                    .Get();
            } catch (const std::exception& e) {
                LOG_ERROR() << e;
                FAIL() << e.what();
            }
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);

    engine::AsyncNoSpan(
        [test_deadline](auto&& tls_client) {
            char c = 0;
            EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('1', c);
            EXPECT_EQ(1, tls_client.SendAll("2", 1, test_deadline));
        },
        std::move(tls_client)
    )
        .Get();

    server_task.Get();
}

UTEST(TlsWrapper, ConnectTimeout) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);
    EXPECT_THROW(
        static_cast<void>(io::TlsWrapper::StartTlsClient(std::move(client), {}, Deadline::FromDuration(kShortTimeout))),
        io::IoTimeout
    );
    EXPECT_THROW(
        static_cast<void>(io::TlsWrapper::StartTlsServer(
            std::move(server),
            crypto::LoadCertificatesChainFromString(cert),
            crypto::PrivateKey::LoadFromString(key),
            Deadline::FromDuration(kShortTimeout)
        )),
        io::IoException
    );
}

UTEST_MT(TlsWrapper, IoTimeout, 2) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    engine::SingleConsumerEvent timeout_happened;
    auto server_task = engine::AsyncNoSpan(
        [test_deadline, &timeout_happened](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert),
                crypto::PrivateKey::LoadFromString(key),
                test_deadline
            );
            char c = 0;
            UEXPECT_THROW(
                static_cast<void>(tls_server.RecvSome(&c, 1, Deadline::FromDuration(kShortTimeout))), io::IoTimeout
            );
            timeout_happened.Send();
        // OpenSSL 1.0 always breaks the channel here. Please update.
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
            EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('1', c);
#endif
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
    ASSERT_TRUE(timeout_happened.WaitForEventUntil(test_deadline));
    // see above
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    EXPECT_EQ(1, tls_client.SendAll("1", 1, test_deadline));
#endif
    server_task.Get();
}

UTEST(TlsWrapper, Cancel) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert),
                crypto::PrivateKey::LoadFromString(key),
                test_deadline
            );
            char c = 0;
            UEXPECT_THROW(static_cast<void>(tls_server.RecvSome(&c, 1, test_deadline)), io::IoInterrupted);
        },
        std::move(server)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);

    engine::Yield();
    server_task.SyncCancel();
}

UTEST_MT(TlsWrapper, CertKeyMismatch, 2) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            UEXPECT_THROW(
                static_cast<void>(io::TlsWrapper::StartTlsServer(
                    std::forward<decltype(server)>(server),
                    crypto::LoadCertificatesChainFromString(cert),
                    crypto::PrivateKey::LoadFromString(other_key),
                    test_deadline
                )),
                io::TlsException
            );
        },
        std::move(server)
    );

    EXPECT_THROW(
        static_cast<void>(io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline)), io::IoException
    );
    server_task.Get();
}

UTEST_MT(TlsWrapper, NonTlsClient, 2) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            UEXPECT_THROW(
                static_cast<void>(io::TlsWrapper::StartTlsServer(
                    std::forward<decltype(server)>(server),
                    crypto::LoadCertificatesChainFromString(cert),
                    crypto::PrivateKey::LoadFromString(other_key),
                    test_deadline
                )),
                io::TlsException
            );
        },
        std::move(server)
    );

    EXPECT_EQ(5, client.SendAll("hello", 5, test_deadline));
    server_task.Get();
}

UTEST_MT(TlsWrapper, NonTlsServer, 2) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) { EXPECT_EQ(5, server.SendAll("hello", 5, test_deadline)); }, std::move(server)
    );

    EXPECT_THROW(
        static_cast<void>(io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline)), io::IoException
    );
    server_task.Get();
}

UTEST_MT(TlsWrapper, DoubleSmoke, 4) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);
    auto [other_server, other_client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(cert),
                crypto::PrivateKey::LoadFromString(key),
                test_deadline
            );
            EXPECT_EQ(1, tls_server.SendAll("1", 1, test_deadline));
            char c = 0;
            EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('2', c);

            auto raw_server = tls_server.StopTls(test_deadline);
            EXPECT_EQ(1, raw_server.SendAll("3", 1, test_deadline));
            EXPECT_EQ(1, raw_server.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('4', c);
        },
        std::move(server)
    );

    auto other_server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            auto tls_server = io::TlsWrapper::StartTlsServer(
                std::forward<decltype(server)>(server),
                crypto::LoadCertificatesChainFromString(other_cert),
                crypto::PrivateKey::LoadFromString(other_key),
                test_deadline
            );
            EXPECT_EQ(1, tls_server.SendAll("5", 1, test_deadline));
            char c = 0;
            EXPECT_EQ(1, tls_server.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('6', c);

            auto raw_server = tls_server.StopTls(test_deadline);
            EXPECT_EQ(1, raw_server.SendAll("7", 1, test_deadline));
            EXPECT_EQ(1, raw_server.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('8', c);
        },
        std::move(other_server)
    );

    auto other_client_task = engine::AsyncNoSpan(
        [test_deadline](auto&& client) {
            auto tls_client = io::TlsWrapper::StartTlsClient(std::forward<decltype(client)>(client), {}, test_deadline);
            char c = 0;
            EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('5', c);
            EXPECT_EQ(1, tls_client.SendAll("6", 1, test_deadline));
            auto raw_client = tls_client.StopTls(test_deadline);
            EXPECT_EQ(1, raw_client.RecvSome(&c, 1, test_deadline));
            EXPECT_EQ('7', c);
            EXPECT_EQ(1, raw_client.SendAll("8", 1, test_deadline));
        },
        std::move(other_client)
    );

    auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
    char c = 0;
    EXPECT_EQ(1, tls_client.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('1', c);
    EXPECT_EQ(1, tls_client.SendAll("2", 1, test_deadline));
    auto raw_client = tls_client.StopTls(test_deadline);
    EXPECT_EQ(1, raw_client.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('3', c);
    EXPECT_EQ(1, raw_client.SendAll("4", 1, test_deadline));

    server_task.Get();
    other_server_task.Get();
    other_client_task.Get();
}

UTEST(TlsWrapper, InvalidSocket) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    UEXPECT_THROW(static_cast<void>(io::TlsWrapper::StartTlsClient({}, {}, test_deadline)), io::TlsException);
    UEXPECT_THROW(
        static_cast<void>(io::TlsWrapper::StartTlsServer(
            {}, crypto::LoadCertificatesChainFromString(cert), crypto::PrivateKey::LoadFromString(key), test_deadline
        )),
        io::TlsException
    );
}

UTEST(TlsWrapper, PeerShutdown) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            try {
                auto tls_server = io::TlsWrapper::StartTlsServer(
                    std::forward<decltype(server)>(server),
                    crypto::LoadCertificatesChainFromString(cert),
                    crypto::PrivateKey::LoadFromString(key),
                    test_deadline
                );
                char c = 0;
                // Get a non-fatal error on the channel
                EXPECT_THROW(
                    [[maybe_unused]] const auto ret =
                        tls_server.RecvAll(&c, 1, engine::Deadline::FromDuration(std::chrono::milliseconds{1})),
                    io::IoTimeout
                );
                ASSERT_EQ(1, tls_server.SendAll(&c, 1, test_deadline));
                EXPECT_EQ(0, tls_server.RecvSome(&c, 1, test_deadline));
            } catch (const std::exception& e) {
                LOG_ERROR() << e;
                FAIL() << e.what();
            }
        },
        std::move(server)
    );

    {
        auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
        char c = 0;
        ASSERT_EQ(1, tls_client.RecvAll(&c, 1, test_deadline));
        // destroy the wrapper causing an unidirectional shutdown
    }

    server_task.Get();
}

UTEST(TlsWrapper, PeerDisconnect) {
    const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

    TcpListener tcp_listener;
    auto [server, client] = tcp_listener.MakeSocketPair(test_deadline);

    auto server_task = engine::AsyncNoSpan(
        [test_deadline](auto&& server) {
            try {
                auto tls_server = io::TlsWrapper::StartTlsServer(
                    std::forward<decltype(server)>(server),
                    crypto::LoadCertificatesChainFromString(cert),
                    crypto::PrivateKey::LoadFromString(key),
                    test_deadline
                );
                char c = 0;
                // Get a non-fatal error on the channel
                EXPECT_THROW(
                    [[maybe_unused]] const auto ret =
                        tls_server.RecvAll(&c, 1, engine::Deadline::FromDuration(std::chrono::milliseconds{1})),
                    io::IoTimeout
                );
                ASSERT_EQ(1, tls_server.SendAll(&c, 1, test_deadline));
                EXPECT_THROW(
                    [[maybe_unused]] const auto ret = tls_server.RecvAll(&c, 1, test_deadline), io::TlsException
                );
            } catch (const std::exception& e) {
                LOG_ERROR() << e;
                FAIL() << e.what();
            }
        },
        std::move(server)
    );

    {
        const auto client_fd = client.Fd();
        auto tls_client = io::TlsWrapper::StartTlsClient(std::move(client), {}, test_deadline);
        char c = 0;
        ASSERT_EQ(1, tls_client.RecvAll(&c, 1, test_deadline));
        // disconnect the underlying socket without shutting down the channel
        ::shutdown(client_fd, SHUT_RDWR);
    }

    server_task.Get();
}

USERVER_NAMESPACE_END
