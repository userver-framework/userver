#include <userver/clients/http/expected_status_codes.hpp>
#include <userver/clients/http/response.hpp>

#include <optional>
#include <stdexcept>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

constexpr ExpectedStatusCodes kEmptyStatusCodes{};
constexpr std::optional<ExpectedStatusCodes> kUnsetStatusCodes{};
constexpr std::optional<ExpectedStatusCodes> kClearedStatusCodes{kEmptyStatusCodes};
static_assert(!kUnsetStatusCodes.has_value());
static_assert(kClearedStatusCodes.has_value());

}  // namespace

TEST(ExpectedStatusCodes, ContainsOnlyConfiguredCodes) {
    const ExpectedStatusCodes codes{
        USERVER_NAMESPACE::http::StatusCode::kContinue,
        USERVER_NAMESPACE::http::StatusCode::kNotFound,
        USERVER_NAMESPACE::http::StatusCode::kNotFound,
        static_cast<USERVER_NAMESPACE::http::StatusCode>(599)
    };
    EXPECT_TRUE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kContinue));
    EXPECT_TRUE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kNotFound));
    EXPECT_TRUE(codes.Contains(static_cast<USERVER_NAMESPACE::http::StatusCode>(599)));
    EXPECT_FALSE(codes.Contains(static_cast<USERVER_NAMESPACE::http::StatusCode>(99)));
    EXPECT_FALSE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kForbidden));
    EXPECT_FALSE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kInternalServerError));
    EXPECT_FALSE(codes.Contains(static_cast<USERVER_NAMESPACE::http::StatusCode>(600)));
    EXPECT_FALSE(kEmptyStatusCodes.Contains(USERVER_NAMESPACE::http::StatusCode::kNotFound));
}

TEST(ExpectedStatusCodes, OwnsCopiedSetAndIgnoresOrderAndDuplicates) {
    ExpectedStatusCodes
        codes{USERVER_NAMESPACE::http::StatusCode::kNotFound, USERVER_NAMESPACE::http::StatusCode::kConflict};
    const auto copy = codes;
    codes = ExpectedStatusCodes{};
    EXPECT_TRUE(copy.Contains(USERVER_NAMESPACE::http::StatusCode::kNotFound));
    EXPECT_TRUE(copy.Contains(USERVER_NAMESPACE::http::StatusCode::kConflict));
    EXPECT_FALSE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kNotFound));
    EXPECT_EQ(
        copy,
        (ExpectedStatusCodes{
            USERVER_NAMESPACE::http::StatusCode::kConflict,
            USERVER_NAMESPACE::http::StatusCode::kNotFound,
            USERVER_NAMESPACE::http::StatusCode::kConflict
        })
    );
    EXPECT_NE(copy, (ExpectedStatusCodes{USERVER_NAMESPACE::http::StatusCode::kNotFound}));
}

TEST(ExpectedStatusCodes, RejectsInvalidCodes) {
    EXPECT_THROW((ExpectedStatusCodes{static_cast<USERVER_NAMESPACE::http::StatusCode>(99)}), std::invalid_argument);
    EXPECT_THROW((ExpectedStatusCodes{static_cast<USERVER_NAMESPACE::http::StatusCode>(600)}), std::invalid_argument);
    EXPECT_THROW(
        (ExpectedStatusCodes{
            USERVER_NAMESPACE::http::StatusCode::kNotFound,
            static_cast<USERVER_NAMESPACE::http::StatusCode>(-1)
        }),
        std::invalid_argument
    );
}

TEST(ExpectedStatusCodes, CanAddAndRemoveCodesDynamically) {
    ExpectedStatusCodes codes;
    codes.Set(USERVER_NAMESPACE::http::StatusCode::kNotFound, true);
    EXPECT_TRUE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kNotFound));
    codes.Set(USERVER_NAMESPACE::http::StatusCode::kNotFound, true);
    codes.Set(USERVER_NAMESPACE::http::StatusCode::kConflict, true);
    codes.Set(USERVER_NAMESPACE::http::StatusCode::kNotFound, false);
    EXPECT_FALSE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kNotFound));
    EXPECT_TRUE(codes.Contains(USERVER_NAMESPACE::http::StatusCode::kConflict));
}

TEST(ExpectedStatusCodes, DynamicallyRejectsInvalidCodes) {
    ExpectedStatusCodes codes{USERVER_NAMESPACE::http::StatusCode::kConflict};
    const auto copy = codes;
    EXPECT_THROW(codes.Set(static_cast<USERVER_NAMESPACE::http::StatusCode>(99), true), std::invalid_argument);
    EXPECT_EQ(codes, copy);
    EXPECT_THROW(codes.Set(static_cast<USERVER_NAMESPACE::http::StatusCode>(600), false), std::invalid_argument);
    EXPECT_EQ(codes, copy);
}

TEST(ExpectedStatusCodes, ResponseClassificationDoesNotChangeStatusOrExceptions) {
    Response response;
    response.SetStatusCode(USERVER_NAMESPACE::http::StatusCode::kNotFound);
    EXPECT_TRUE(response.IsError());
    response.SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kNotFound});
    EXPECT_FALSE(response.IsError());
    EXPECT_EQ(response.status_code(), USERVER_NAMESPACE::http::StatusCode::kNotFound);
    EXPECT_THROW(response.raise_for_status(), HttpClientException);
    response.SetStatusCode(USERVER_NAMESPACE::http::StatusCode::kInternalServerError);
    EXPECT_TRUE(response.IsError());
    response.SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kInternalServerError});
    EXPECT_FALSE(response.IsError());
    EXPECT_THROW(response.raise_for_status(), HttpServerException);
    response.SetExpectedStatusCodes({});
    EXPECT_TRUE(response.IsError());
}

}  // namespace clients::http

USERVER_NAMESPACE_END
