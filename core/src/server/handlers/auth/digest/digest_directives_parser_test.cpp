#include <userver/server/handlers/auth/digest/digest_directives_parser.hpp>
#include <userver/utest/utest.hpp>

#include <exception>
#include <string_view>

USERVER_NAMESPACE_BEGIN

using Parser = server::handlers::auth::digest::Parser;

TEST(AuthenticationInfoCorrectParsing, WithoutOptional) {
  std::string_view correctInfo = R"(username="Mufasa",
        realm="testrealm@host.com",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        response="6629fae49393a05397450978507c4ef1"
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));
  auto auth_context = parser.GetClientContext();

  EXPECT_EQ(auth_context.username, "Mufasa");
  EXPECT_EQ(auth_context.nonce, "dcd98b7102dd2f0e8b11d0f600bfb0c093");
  EXPECT_EQ(auth_context.realm, "testrealm@host.com");
  EXPECT_EQ(auth_context.uri, "/dir/index.html");
  EXPECT_EQ(auth_context.response, "6629fae49393a05397450978507c4ef1");
}

TEST(AuthenticationInfoCorrectParsing, WithOptionalPartial) {
  std::string_view correctInfo = R"(username="Mufasa",
        realm="testrealm@host.com",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        response="6629fae49393a05397450978507c4ef1",
        algorithm=MD5
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));

  auto auth_context = parser.GetClientContext();

  EXPECT_EQ(auth_context.username, "Mufasa");
  EXPECT_EQ(auth_context.nonce, "dcd98b7102dd2f0e8b11d0f600bfb0c093");
  EXPECT_EQ(auth_context.realm, "testrealm@host.com");
  EXPECT_EQ(auth_context.uri, "/dir/index.html");
  EXPECT_EQ(auth_context.response, "6629fae49393a05397450978507c4ef1");
  EXPECT_EQ(auth_context.algorithm, "MD5");
}

TEST(AuthenticationInfoCorrectParsing, WithOptionalAll) {
  std::string_view correctInfo = R"(username="Mufasa",
        realm="testrealm@host.com",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        qop=auth,
        nc=00000001,
        cnonce="0a4f113b",
        response="6629fae49393a05397450978507c4ef1",
        opaque="5ccc069c403ebaf9f0171e9517f40e41",
        auth-param="fictional parameter"
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));

  auto auth_context = parser.GetClientContext();

  EXPECT_EQ(auth_context.username, "Mufasa");
  EXPECT_EQ(auth_context.nonce, "dcd98b7102dd2f0e8b11d0f600bfb0c093");
  EXPECT_EQ(auth_context.realm, "testrealm@host.com");
  EXPECT_EQ(auth_context.uri, "/dir/index.html");
  EXPECT_EQ(auth_context.response, "6629fae49393a05397450978507c4ef1");

  EXPECT_TRUE(!auth_context.algorithm.empty());
  EXPECT_EQ(auth_context.algorithm, "MD5");

  EXPECT_TRUE(!auth_context.opaque.empty());
  EXPECT_EQ(auth_context.opaque, "5ccc069c403ebaf9f0171e9517f40e41");

  EXPECT_TRUE(!auth_context.nc.empty());
  EXPECT_EQ(auth_context.nc, "00000001");

  EXPECT_TRUE(!auth_context.cnonce.empty());
  EXPECT_EQ(auth_context.cnonce, "0a4f113b");

  EXPECT_TRUE(!auth_context.qop.empty());
  EXPECT_EQ(auth_context.qop, "auth");

  EXPECT_TRUE(!auth_context.authparam.empty());
  EXPECT_EQ(auth_context.authparam, "fictional parameter");
}

TEST(AuthenticationInfoIncorrectParsing, MandatoryDirectivesMissing) {
  std::string_view correctInfo = R"(algorithm=MD5,
        qop=auth,
        nc=00000001,
        cnonce="0a4f113b",
        response="6629fae49393a05397450978507c4ef1",
        opaque="5ccc069c403ebaf9f0171e9517f40e41",
        auth-param="fictional parameter"
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));
  EXPECT_THROW(parser.GetClientContext(), std::runtime_error);
}

TEST(AuthenticationInfoIncorrectParsing, MandatoryDirectiveMissing) {
  std::string_view correctInfo = R"(username="Mufasa",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        qop=auth,
        nc=00000001,
        cnonce="0a4f113b",
        response="6629fae49393a05397450978507c4ef1",
        opaque="5ccc069c403ebaf9f0171e9517f40e41",
        auth-param="fictional parameter"
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));
  EXPECT_THROW(parser.GetClientContext(), std::runtime_error);
}

TEST(AuthenticationInfoIncorrectParsing, MandatoryDirectiveMissingExtended) {
  std::string_view correctInfo = R"(username="Mufasa",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        qop=auth,
        nc=00000001,
        cnonce="0a4f113b",
        response="6629fae49393a05397450978507c4ef1",
        opaque="5ccc069c403ebaf9f0171e9517f40e41",
        auth-param="fictional parameter"
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));
  EXPECT_THROW(parser.GetClientContext(), std::runtime_error);
}

// Value Parsing Errors
TEST(AuthenticationInfoIncorrectParsing, MandatoryDirectiveNoValue) {
  std::string_view correctInfo = R"(username=,
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        response="6629fae49393a05397450978507c4ef1",
    )";
  Parser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo), std::runtime_error);
}

TEST(AuthenticationInfoIncorrectParsing, OptionalDirectiveNoValue) {
  std::string_view correctInfo = R"(username="Mufasa",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        response="6629fae49393a05397450978507c4ef1",
        qop=,
    )";
  Parser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo), std::runtime_error);
}

// Directory Parsing Errors
TEST(AuthenticationInfoIncorrectParsing, InvalidMandatoryDirectory) {
  std::string_view correctInfo = R"(usergame="Mubasa",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        response="6629fae49393a05397450978507c4ef1"
    )";
  Parser parser;
  EXPECT_NO_THROW(parser.ParseAuthInfo(correctInfo));
  EXPECT_THROW(parser.GetClientContext(), std::runtime_error);
}

USERVER_NAMESPACE_END
