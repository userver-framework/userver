#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/percentile.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

static_assert(utils::statistics::HasWriterSupport<utils::statistics::Percentile<100>>);

TEST(Percentile, Zero) {
    const utils::statistics::Percentile<100> p;

    EXPECT_EQ(0U, p.GetPercentile(0));
    EXPECT_EQ(0U, p.GetPercentile(50));
    EXPECT_EQ(0U, p.GetPercentile(100));
}

TEST(Percentile, One) {
    utils::statistics::Percentile<100> p;

    p.Account(3);

    EXPECT_EQ(3U, p.GetPercentile(0));
    EXPECT_EQ(3U, p.GetPercentile(50));
    EXPECT_EQ(3U, p.GetPercentile(100));
}

TEST(Percentile, Hundred) {
    utils::statistics::Percentile<100> p;

    for (int i = 0; i < 100; i++) {
        p.Account(i);
    }

    EXPECT_EQ(0U, p.GetPercentile(0));
    EXPECT_EQ(50U, p.GetPercentile(50));
    EXPECT_EQ(99U, p.GetPercentile(100));
    EXPECT_EQ(99U, p.GetPercentile(101));
    EXPECT_EQ(99U, p.GetPercentile(200));
}

TEST(Percentile, Many) {
    utils::statistics::Percentile<3> p;

    p.Account(0);
    p.Account(0);
    p.Account(0);
    p.Account(1);
    p.Account(1);

    EXPECT_EQ(0U, p.GetPercentile(0));
    EXPECT_EQ(0U, p.GetPercentile(50));
    EXPECT_EQ(1U, p.GetPercentile(100));
    EXPECT_EQ(1U, p.GetPercentile(101));
    EXPECT_EQ(1U, p.GetPercentile(200));
}

TEST(Percentile, Extra100perc) {
    utils::statistics::Percentile<10, unsigned int, 10, 10> p;

    p.Account(0);
    EXPECT_EQ(0U, p.GetPercentile(100));

    p.Account(7);
    EXPECT_EQ(7U, p.GetPercentile(100));

    p.Account(9);
    EXPECT_EQ(9U, p.GetPercentile(100));

    p.Account(10);
    EXPECT_EQ(10U, p.GetPercentile(100));

    p.Account(13);
    EXPECT_EQ(10U, p.GetPercentile(100));

    p.Account(19);
    EXPECT_EQ(20U, p.GetPercentile(100));

    p.Account(21);
    EXPECT_EQ(20U, p.GetPercentile(100));

    p.Account(100);
    EXPECT_EQ(100U, p.GetPercentile(100));

    p.Account(101);
    EXPECT_EQ(100U, p.GetPercentile(100));

    p.Account(201);
    EXPECT_EQ(100U, p.GetPercentile(100));
}

TEST(Percentile, FieldName) {
    EXPECT_EQ("p0", utils::statistics::GetPercentileFieldName(0));
    EXPECT_EQ("p11_1", utils::statistics::GetPercentileFieldName(11.1));
    EXPECT_EQ("p25", utils::statistics::GetPercentileFieldName(25));
    EXPECT_EQ("p33_3", utils::statistics::GetPercentileFieldName(33.3));
    EXPECT_EQ("p50", utils::statistics::GetPercentileFieldName(50.0));
    EXPECT_EQ("p75", utils::statistics::GetPercentileFieldName(75));
    EXPECT_EQ("p77_7", utils::statistics::GetPercentileFieldName(77.7));
    EXPECT_EQ("p90", utils::statistics::GetPercentileFieldName(90));
    EXPECT_EQ("p95", utils::statistics::GetPercentileFieldName(95));
    EXPECT_EQ("p97_3", utils::statistics::GetPercentileFieldName(97.3));
    EXPECT_EQ("p98", utils::statistics::GetPercentileFieldName(98));
    EXPECT_EQ("p99", utils::statistics::GetPercentileFieldName(99));
    EXPECT_EQ("p99_4", utils::statistics::GetPercentileFieldName(99.4));
    EXPECT_EQ("p99_6", utils::statistics::GetPercentileFieldName(99.6));
    EXPECT_EQ("p99_9", utils::statistics::GetPercentileFieldName(99.9));
    EXPECT_EQ("p100", utils::statistics::GetPercentileFieldName(100));
}

USERVER_NAMESPACE_END
