#include <gtest/gtest.h>

#include <functional>

#include <userver/http/content_type.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ContentType, Smoke) {
    const http::ContentType content_type("text/html");

    EXPECT_EQ("text/html", content_type.MediaType());
    EXPECT_EQ("text", content_type.TypeToken());
    EXPECT_EQ("html", content_type.SubtypeToken());
    EXPECT_FALSE(content_type.HasExplicitCharset());
    EXPECT_EQ("UTF-8", content_type.Charset());
    EXPECT_EQ(1000, content_type.Quality());
    EXPECT_EQ("text/html", content_type.ToString());
    EXPECT_EQ(content_type, content_type);
}

TEST(ContentType, Invalid) {
    EXPECT_THROW(http::ContentType(""), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("text"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("http/"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("/text"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html /text"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/ text"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text/yes"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text, charset=utf-8"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text;; charset=utf-8"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; ; charset=utf-8"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; charset="), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; utf-8"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; =utf-8"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; q=utf-8"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; q=-1"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; q=1000"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; q=2.0"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; q=0.0001"), http::MalformedContentType);
    EXPECT_THROW(http::ContentType("html/text; q=1.001"), http::MalformedContentType);
}

TEST(ContentType, SkipUnknown) {
    const http::ContentType content_type(
        "text/html; some_option=value; charset=utf-8; v=2; q=0; "
        "other_option=123"
    );

    EXPECT_EQ("text/html", content_type.MediaType());
    EXPECT_EQ("text", content_type.TypeToken());
    EXPECT_EQ("html", content_type.SubtypeToken());
    EXPECT_TRUE(content_type.HasExplicitCharset());
    EXPECT_EQ("utf-8", content_type.Charset());
    EXPECT_EQ(0, content_type.Quality());
}

TEST(ContentType, Icase) {
    const http::ContentType lower("text/html");
    const http::ContentType upper("TEXT/HTML");
    const http::ContentType mixed("TeXt/HtMl");

    EXPECT_EQ(lower, upper);
    EXPECT_EQ(lower, mixed);
    EXPECT_EQ(mixed, upper);

    const http::ContentType with_cs("text/html; CharSet=uTF-8");
    EXPECT_TRUE(with_cs.HasExplicitCharset());
    EXPECT_EQ("uTF-8", with_cs.Charset());
    EXPECT_EQ(mixed, with_cs);
}

TEST(ContentType, Charset) {
    const http::ContentType impli("application/json");
    const http::ContentType expli("application/json; charset=utf-8");
    const http::ContentType other("application/json;charset=koi8-r");

    EXPECT_FALSE(impli.HasExplicitCharset());
    EXPECT_EQ("UTF-8", impli.Charset());
    EXPECT_TRUE(impli.ToString().find("charset=") == std::string::npos);
    EXPECT_TRUE(expli.HasExplicitCharset());
    EXPECT_EQ("utf-8", expli.Charset());
    EXPECT_FALSE(expli.ToString().find("charset=utf-8") == std::string::npos);
    EXPECT_EQ(impli, expli);

    EXPECT_TRUE(other.HasExplicitCharset());
    EXPECT_EQ("koi8-r", other.Charset());
    EXPECT_NE(impli, other);
    EXPECT_NE(expli, other);
}

TEST(ContentType, Quality) {
    const http::ContentType impli("text/html");
    const http::ContentType max0("text/html; q=1");
    const http::ContentType max1("text/html; q=1.");
    const http::ContentType max2("text/html; q=1.0");
    const http::ContentType max3("text/html; q=1.00");
    const http::ContentType max4("text/html; q=1.000");

    EXPECT_EQ(1000, impli.Quality());
    EXPECT_EQ(1000, max0.Quality());
    EXPECT_EQ(1000, max1.Quality());
    EXPECT_EQ(1000, max2.Quality());
    EXPECT_EQ(1000, max3.Quality());
    EXPECT_EQ(1000, max4.Quality());

    EXPECT_EQ(impli, max0);
    EXPECT_EQ(max0, max1);
    EXPECT_EQ(max1, max2);
    EXPECT_EQ(max2, max3);
    EXPECT_EQ(max3, max4);

    EXPECT_TRUE(impli.ToString().find("q=") == std::string::npos);
    // max quality may be skipped if specified

    const http::ContentType min1("text/html; q=0.");
    EXPECT_EQ(0, min1.Quality());
    EXPECT_FALSE(min1.ToString().find("q=0.000") == std::string::npos);

    const http::ContentType other1("text/html; q=0.1");
    const http::ContentType other2("text/html; q=0.397");
    EXPECT_EQ(100, other1.Quality());
    EXPECT_FALSE(other1.ToString().find("q=0.100") == std::string::npos);
    EXPECT_EQ(397, other2.Quality());
    EXPECT_FALSE(other2.ToString().find("q=0.397") == std::string::npos);

    EXPECT_NE(other1, other2);
    EXPECT_NE(max0, other1);
    EXPECT_NE(max0, other2);
}

TEST(ContentType, Ordering) {
    const http::ContentType any_mt("*/*");
    const http::ContentType any_subtype("text/*");
    const http::ContentType type("text/html; q=0.9");
    const http::ContentType type_less_q("text/html; q=0.7");
    const http::ContentType type_charset("text/html; charset=utf-8; q=0.2");
    const http::ContentType type_charset_less_q("text/html; charset=utf-8; q=0.02");
    const http::ContentType mixed_case("TeXt/HtMl; Q=0.9");
    const http::ContentType app_json("application/json");
    const http::ContentType app_json_charset("application/json; charset=utf-8");
    const http::ContentType app_json_charset_q("application/json; charset=utf-8; q=0.");

    EXPECT_TRUE(any_mt < any_subtype);
    EXPECT_TRUE(any_mt < type);
    EXPECT_TRUE(any_mt < type_less_q);
    EXPECT_TRUE(any_mt < type_charset);
    EXPECT_TRUE(any_mt < type_charset_less_q);
    EXPECT_TRUE(any_mt < mixed_case);
    EXPECT_TRUE(any_mt < app_json);
    EXPECT_TRUE(any_mt < app_json_charset);
    EXPECT_TRUE(any_mt < app_json_charset_q);

    EXPECT_FALSE(any_subtype < any_mt);
    EXPECT_TRUE(any_subtype < type);
    EXPECT_TRUE(any_subtype < type_less_q);
    EXPECT_TRUE(any_subtype < type_charset);
    EXPECT_TRUE(any_subtype < type_charset_less_q);
    EXPECT_TRUE(any_subtype < mixed_case);

    EXPECT_FALSE(type < any_mt);
    EXPECT_FALSE(type < any_subtype);
    EXPECT_FALSE(type < type_less_q);
    EXPECT_TRUE(type < type_charset);
    EXPECT_TRUE(type < type_charset_less_q);
    EXPECT_FALSE(type < mixed_case);

    EXPECT_FALSE(type_less_q < any_mt);
    EXPECT_FALSE(type_less_q < any_subtype);
    EXPECT_TRUE(type_less_q < type);
    EXPECT_TRUE(type_less_q < type_charset);
    EXPECT_TRUE(type_less_q < type_charset_less_q);
    EXPECT_TRUE(type_less_q < mixed_case);

    EXPECT_FALSE(type_charset < any_mt);
    EXPECT_FALSE(type_charset < any_subtype);
    EXPECT_FALSE(type_charset < type);
    EXPECT_FALSE(type_charset < type_less_q);
    EXPECT_FALSE(type_charset < type_charset_less_q);
    EXPECT_FALSE(type_charset < mixed_case);

    EXPECT_FALSE(type_charset_less_q < any_mt);
    EXPECT_FALSE(type_charset_less_q < any_subtype);
    EXPECT_FALSE(type_charset_less_q < type);
    EXPECT_FALSE(type_charset_less_q < type_less_q);
    EXPECT_TRUE(type_charset_less_q < type_charset);
    EXPECT_FALSE(type_charset_less_q < mixed_case);

    EXPECT_FALSE(mixed_case < any_mt);
    EXPECT_FALSE(mixed_case < any_subtype);
    EXPECT_FALSE(mixed_case < type);
    EXPECT_FALSE(mixed_case < type_less_q);
    EXPECT_TRUE(mixed_case < type_charset);
    EXPECT_TRUE(mixed_case < type_charset_less_q);

    EXPECT_FALSE(app_json < any_mt);
    EXPECT_TRUE(app_json < app_json_charset);
    EXPECT_TRUE(app_json < app_json_charset_q);

    EXPECT_FALSE(app_json_charset < any_mt);
    EXPECT_FALSE(app_json_charset < app_json);
    EXPECT_FALSE(app_json_charset < app_json_charset_q);

    EXPECT_FALSE(app_json_charset_q < any_mt);
    EXPECT_FALSE(app_json_charset_q < app_json);
    EXPECT_TRUE(app_json_charset_q < app_json_charset);
}

TEST(ContentType, Hashing) {
    const http::ContentTypeHash hasher;

    auto any_mt_hash = hasher("*/*");
    auto any_subtype_hash = hasher("text/*");
    auto type_hash = hasher("text/html; q=0.9");
    auto type_less_q_hash = hasher("text/html; q=0.7");
    auto type_charset_hash = hasher("text/html; charset=utf-8; q=0.2");
    auto type_charset_less_q_hash = hasher("text/html; charset=utf-8; q=0.02");
    auto mixed_case_hash = hasher("TeXt/HtMl; Q=0.9");

    EXPECT_NE(any_mt_hash, any_subtype_hash);
    EXPECT_NE(any_mt_hash, type_hash);
    EXPECT_NE(any_mt_hash, type_less_q_hash);
    EXPECT_NE(any_mt_hash, type_charset_hash);
    EXPECT_NE(any_mt_hash, type_charset_less_q_hash);
    EXPECT_NE(any_mt_hash, mixed_case_hash);

    EXPECT_NE(any_subtype_hash, type_hash);
    EXPECT_NE(any_subtype_hash, type_less_q_hash);
    EXPECT_NE(any_subtype_hash, type_charset_hash);
    EXPECT_NE(any_subtype_hash, type_charset_less_q_hash);
    EXPECT_NE(any_subtype_hash, mixed_case_hash);

    EXPECT_NE(type_hash, type_less_q_hash);
    EXPECT_NE(type_hash, type_charset_hash);
    EXPECT_NE(type_hash, type_charset_less_q_hash);
    EXPECT_EQ(type_hash, mixed_case_hash);

    EXPECT_NE(type_less_q_hash, type_charset_hash);
    EXPECT_NE(type_less_q_hash, type_charset_less_q_hash);
    EXPECT_NE(type_less_q_hash, mixed_case_hash);

    EXPECT_NE(type_charset_hash, type_charset_less_q_hash);
    EXPECT_NE(type_charset_hash, mixed_case_hash);

    EXPECT_NE(type_charset_less_q_hash, mixed_case_hash);
}

USERVER_NAMESPACE_END
