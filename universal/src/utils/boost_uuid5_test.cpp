#include <userver/utils/boost_uuid4.hpp>
#include <userver/utils/boost_uuid5.hpp>

#include <string>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(UUID, BoostV5UrlNamespaceMatchesRfcVector) {
    const std::string name = "https://example.com/product/image.png";

    const auto once = utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, name);

    EXPECT_EQ(utils::ToString(once), "ae19bb34-8126-55bf-bb41-f73c17cee8a3");
    EXPECT_EQ(once, utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, name));
}

TEST(UUID, BoostV5UrlNamespaceEmptyNameMatchesRfcVector) {
    const auto once = utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, "");
    EXPECT_EQ(utils::ToString(once), "1b4db7eb-4057-5ddf-91e0-36dec72071f5");
    EXPECT_EQ(once, utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, ""));
}

TEST(UUID, BoostV5DifferentNamesYieldDifferentUuids) {
    EXPECT_NE(
        utils::generators::GenerateBoostUuidV5(utils::generators::ns::kDns, "a"),
        utils::generators::GenerateBoostUuidV5(utils::generators::ns::kDns, "b")
    );
    EXPECT_NE(
        utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, "a"),
        utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, "b")
    );
}

TEST(UUID, BoostV5DifferentNamespacesYieldDifferentUuids) {
    constexpr std::string_view kName = "same";
    EXPECT_NE(
        utils::generators::GenerateBoostUuidV5(utils::generators::ns::kDns, kName),
        utils::generators::GenerateBoostUuidV5(utils::generators::ns::kUrl, kName)
    );
}

USERVER_NAMESPACE_END
