#include <gtest/gtest.h>

#include <exception>
#include <stdexcept>
#include <string>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/utils/aimd_limiter.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

TEST(AimdLimiter, InitialState) {
    utils::AimdLimiter limiter{100, {}};
    EXPECT_EQ(100, limiter.GetMaxLimit());
    EXPECT_EQ(100, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, InvalidPolicy) {
    EXPECT_THROW(utils::AimdLimiter(100, {.min_limit = 0}), std::runtime_error);
    EXPECT_THROW(utils::AimdLimiter(100, {.alpha = 0.0}), std::runtime_error);
    EXPECT_THROW(utils::AimdLimiter(100, {.alpha = -1.0}), std::runtime_error);
    EXPECT_THROW(utils::AimdLimiter(100, {.beta = 0.0}), std::runtime_error);
    EXPECT_THROW(utils::AimdLimiter(100, {.beta = 1.0}), std::runtime_error);
    EXPECT_THROW(utils::AimdLimiter(100, {.beta = 1.5}), std::runtime_error);

    utils::AimdLimiter limiter{100, {}};
    EXPECT_THROW(limiter.SetPolicy({.min_limit = 0}), std::runtime_error);
}

TEST(AimdLimiter, DecreaseOnFailure) {
    utils::AimdLimiter limiter{100, {.min_limit = 2, .alpha = 1.0, .beta = 0.5}};

    limiter.OnFailure();
    EXPECT_EQ(50, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(25, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(12, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(6, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(3, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(2, limiter.GetCurrentLimit());

    // Does not decrease below min_limit
    limiter.OnFailure();
    EXPECT_EQ(2, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, IncreaseOnSuccess) {
    utils::AimdLimiter limiter{100, {.min_limit = 2, .alpha = 1.0, .beta = 0.5}};

    limiter.OnFailure();
    ASSERT_EQ(50, limiter.GetCurrentLimit());

    // limit = 50 + 1/50 = 50.02
    limiter.OnSuccess();
    EXPECT_EQ(50, limiter.GetCurrentLimit());

    // limit^2 grows by ~2 per success, so ~60 successes raise the limit from 50 to 51
    for (int i = 0; i < 60; ++i) {
        limiter.OnSuccess();
    }
    EXPECT_EQ(51, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, IncreaseIsBoundedByMaxLimit) {
    utils::AimdLimiter limiter{4, {.min_limit = 2, .alpha = 10.0, .beta = 0.5}};

    for (int i = 0; i < 100; ++i) {
        limiter.OnSuccess();
    }
    EXPECT_EQ(4, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, SetMaxLimit) {
    utils::AimdLimiter limiter{100, {}};

    limiter.SetMaxLimit(10);
    EXPECT_EQ(10, limiter.GetMaxLimit());
    EXPECT_EQ(10, limiter.GetCurrentLimit());

    // Increasing max limit back does not raise the current limit
    limiter.SetMaxLimit(100);
    EXPECT_EQ(100, limiter.GetMaxLimit());
    EXPECT_EQ(10, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, MaxLimitBelowMinLimit) {
    utils::AimdLimiter limiter{100, {.min_limit = 10, .alpha = 1.0, .beta = 0.5}};

    limiter.SetMaxLimit(5);
    EXPECT_EQ(5, limiter.GetCurrentLimit());

    limiter.OnSuccess();
    EXPECT_EQ(5, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(5, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, ZeroMaxLimit) {
    utils::AimdLimiter limiter{0, {}};
    EXPECT_EQ(0, limiter.GetCurrentLimit());

    // No division by zero or NaN poisoning
    limiter.OnSuccess();
    EXPECT_EQ(0, limiter.GetCurrentLimit());
    limiter.OnFailure();
    EXPECT_EQ(0, limiter.GetCurrentLimit());

    limiter.SetMaxLimit(100);
    limiter.OnSuccess();
    const auto limit = limiter.GetCurrentLimit();
    EXPECT_GE(limit, 2);
    EXPECT_LE(limit, 100);
}

TEST(AimdLimiter, SetPolicy) {
    utils::AimdLimiter limiter{100, {.min_limit = 2, .alpha = 1.0, .beta = 0.5}};

    limiter.SetPolicy({.min_limit = 30, .alpha = 1.0, .beta = 0.9});

    limiter.OnFailure();
    EXPECT_EQ(90, limiter.GetCurrentLimit());

    for (int i = 0; i < 100; ++i) {
        limiter.OnFailure();
    }
    EXPECT_EQ(30, limiter.GetCurrentLimit());
}

TEST(AimdLimiter, RaisingMinLimitClampsCurrentLimit) {
    utils::AimdLimiter limiter{100, {.min_limit = 2, .alpha = 1.0, .beta = 0.5}};

    for (int i = 0; i < 100; ++i) {
        limiter.OnFailure();
    }
    ASSERT_EQ(2, limiter.GetCurrentLimit());

    limiter.SetPolicy({.min_limit = 5, .alpha = 1.0, .beta = 0.5});
    EXPECT_EQ(5, limiter.GetCurrentLimit());
}

namespace {

yaml_config::YamlConfig MakeConfig(const std::string& yaml) {
    return {formats::yaml::FromString(yaml), {}, yaml_config::YamlConfig::Mode::kSecure};
}

}  // namespace

TEST(AimdLimiterConfig, ParsePolicy) {
    const auto config = MakeConfig("min-limit: 7\nalpha: 2.5\nbeta: 0.3\n");
    const auto policy = config.As<utils::AimdLimiter::Policy>();
    EXPECT_EQ(policy.min_limit, 7);
    EXPECT_DOUBLE_EQ(policy.alpha, 2.5);
    EXPECT_DOUBLE_EQ(policy.beta, 0.3);
}

TEST(AimdLimiterConfig, RequireAllFields) {
    const auto config = MakeConfig("min-limit: 7\nalpha: 2.5\n");
    EXPECT_THROW(config.As<utils::AimdLimiter::Policy>(), std::exception);
}

TEST(AimdLimiterConfig, RejectWrongType) {
    const auto config = MakeConfig("min-limit: invalid\nalpha: 2.5\nbeta: 0.3\n");
    EXPECT_THROW(config.As<utils::AimdLimiter::Policy>(), std::exception);
}

USERVER_NAMESPACE_END
