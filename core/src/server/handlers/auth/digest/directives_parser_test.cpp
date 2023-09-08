#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/handlers/auth/exception.hpp>
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
  server::handlers::auth::DigestParser parser;
  server::handlers::auth::DigestContextFromClient auth_context;
  EXPECT_NO_THROW(auth_context = parser.ParseAuthInfo(correctInfo));

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
  server::handlers::auth::DigestParser parser;
  server::handlers::auth::DigestContextFromClient auth_context;
  EXPECT_NO_THROW(auth_context = parser.ParseAuthInfo(correctInfo));

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
  server::handlers::auth::DigestParser parser;
  server::handlers::auth::DigestContextFromClient auth_context;
  EXPECT_NO_THROW(auth_context = parser.ParseAuthInfo(correctInfo));

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

TEST(AuthenticationInfoIncorrectParsing, OneMandatoryDirectiveMissing) {
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
  server::handlers::auth::DigestParser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo),
               server::handlers::auth::MissingDirectivesException);
}

TEST(AuthenticationInfoIncorrectParsing, MultipleMandatoryDirectivesMissing) {
  std::string_view correctInfo = R"(algorithm=MD5,
        qop=auth,
        nc=00000001,
        cnonce="0a4f113b",
        response="6629fae49393a05397450978507c4ef1",
        opaque="5ccc069c403ebaf9f0171e9517f40e41",
        auth-param="fictional parameter"
    )";
  server::handlers::auth::DigestParser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo),
               server::handlers::auth::MissingDirectivesException);
}

TEST(AuthenticationInfoIncorrectParsing, InvalidHeader) {
  std::string_view correctInfo = R"(username=="Mufasa",
        realm="testrea=lm@host.com",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        response="6629fae49393a05397450978507c4ef1",
    )";
  Parser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo),
               server::handlers::auth::ParseException);
}

TEST(AuthenticationInfoIncorrectParsing, UnknownDirective) {
  std::string_view correctInfo = R"(username="Mufasa",
        realm="testrealm@host.com",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        response="6629fae49393a05397450978507c4ef1",
        unknown="some-value"
    )";
  Parser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo),
               server::handlers::auth::ParseException);
}

TEST(AuthenticationInfoIncorrectParsing, InvalidMandatoryDirective) {
  std::string_view correctInfo = R"(usergame="Mubasa",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        response="6629fae49393a05397450978507c4ef1"
    )";
  Parser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo),
               server::handlers::auth::Exception);
}

TEST(AuthenticationInfoIncorrectParsing, DuplicateDirectives) {
  std::string_view correctInfo = R"(username="Mubasa",
        realm="testrealm@host.com",
        username="Alex",
        nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
        uri="/dir/index.html",
        algorithm=MD5,
        response="6629fae49393a05397450978507c4ef1"
    )";
  server::handlers::auth::DigestParser parser;
  EXPECT_THROW(parser.ParseAuthInfo(correctInfo),
               server::handlers::auth::DuplicateDirectiveException);
}

USERVER_NAMESPACE_END
