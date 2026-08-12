#include "helpers.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <userver/utest/utest.hpp>

namespace sqldto_tests {

namespace {

using Color = input::SmokeColor;

input::SmokeProfile MakeProfile() {
    return input::SmokeProfile{
        .display_name = "Alice",
        .age = 30,
        .favourite = Color::kGreen,
        .contacts =
            std::vector<input::SmokeContact>{
                input::SmokeContact{
                    .kind = "email",
                    .value = "alice@example.com",
                    .is_primary = true,
                },
                input::SmokeContact{
                    .kind = "phone",
                    .value = "+1-111-111-1111",
                    .is_primary = false,
                },
            },
    };
}

class PgSmokeTest : public PgSmokeTestBase {};

}  // namespace

UTEST_F(PgSmokeTest, InsertAndCount) {
    const auto host = pg::ClusterHostType::kMaster;

    client_->InsertItem(host, "first", Color::kRed, std::vector<std::string>{"a", "b"}, MakeProfile());
    client_->InsertItem(host, "second", Color::kGreen, std::nullopt, std::nullopt);
    client_->InsertItem(host, "third", Color::kBlue, std::vector<std::string>{"c"}, std::nullopt);

    const auto count = client_->SelectItemCount(host);
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 3);
}

UTEST_F(PgSmokeTest, SelectAllAndArrays) {
    const auto host = pg::ClusterHostType::kMaster;

    client_->InsertItem(host, "first", Color::kRed, std::vector<std::string>{"tag1", "tag2"}, std::nullopt);
    client_->InsertItem(host, "second", Color::kGreen, std::nullopt, std::nullopt);

    const auto all = client_->SelectAllItems(host);
    ASSERT_EQ(all.size(), 2U);

    EXPECT_EQ(all[0].name.value_or(""), "first");
    EXPECT_EQ(all[0].color.value_or(Color{}), Color::kRed);

    const auto& tags = all[0].tags;
    ASSERT_TRUE(tags.has_value());
    ASSERT_EQ(tags->size(), 2U);
    EXPECT_EQ((*tags)[0], "tag1");
    EXPECT_EQ((*tags)[1], "tag2");

    EXPECT_EQ(all[1].name.value_or(""), "second");
    EXPECT_EQ(all[1].color.value_or(Color{}), Color::kGreen);
    EXPECT_FALSE(all[1].tags.has_value());
}

UTEST_F(PgSmokeTest, CompositeRoundTrip) {
    const auto host = pg::ClusterHostType::kMaster;

    const auto id = client_->InsertItem(host, "withprofile", Color::kBlue, std::nullopt, MakeProfile());
    ASSERT_TRUE(id.has_value());

    const auto row = client_->SelectItemById(host, id);
    ASSERT_TRUE(row.has_value());

    const auto& profile = row->profile;
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->display_name.value_or(""), "Alice");
    EXPECT_EQ(profile->age.value_or(0), 30);
    EXPECT_EQ(profile->favourite.value_or(Color{}), Color::kGreen);

    const auto& contacts = profile->contacts;
    ASSERT_TRUE(contacts.has_value());
    ASSERT_EQ(contacts->size(), 2U);
    EXPECT_EQ((*contacts)[0].kind.value_or(""), "email");
    EXPECT_EQ((*contacts)[0].value.value_or(""), "alice@example.com");
    EXPECT_EQ((*contacts)[0].is_primary.value_or(false), true);
    EXPECT_EQ((*contacts)[1].kind.value_or(""), "phone");
    EXPECT_EQ((*contacts)[1].is_primary.value_or(true), false);
}

UTEST_F(PgSmokeTest, SelectByIdMiss) {
    const auto host = pg::ClusterHostType::kMaster;

    const auto missing = client_->SelectItemById(host, std::int64_t{-1});
    EXPECT_FALSE(missing.has_value());
}

UTEST_F(PgSmokeTest, UpdateColor) {
    const auto host = pg::ClusterHostType::kMaster;

    const auto id = client_->InsertItem(host, "item", Color::kRed, std::nullopt, std::nullopt);
    ASSERT_TRUE(id.has_value());
    client_->UpdateItemColor(host, id, Color::kBlue);

    const auto row = client_->SelectItemById(host, id);
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->color.value_or(Color{}), Color::kBlue);
}

UTEST_F(PgSmokeTest, DeleteReturning) {
    const auto host = pg::ClusterHostType::kMaster;

    const auto id = client_->InsertItem(host, "doomed", Color::kRed, std::nullopt, std::nullopt);
    ASSERT_TRUE(id.has_value());

    const auto deleted = client_->DeleteItem(host, id);
    ASSERT_TRUE(deleted.has_value());
    EXPECT_EQ(deleted->id, id);
    EXPECT_EQ(deleted->name.value_or(""), "doomed");

    const auto again = client_->DeleteItem(host, id);
    EXPECT_FALSE(again.has_value());

    const auto count = client_->SelectItemCount(host);
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 0);
}

UTEST_F(PgSmokeTest, CountByColor) {
    const auto host = pg::ClusterHostType::kMaster;

    client_->InsertItem(host, "r1", Color::kRed, std::nullopt, std::nullopt);
    client_->InsertItem(host, "r2", Color::kRed, std::nullopt, std::nullopt);
    client_->InsertItem(host, "g1", Color::kGreen, std::nullopt, std::nullopt);

    const auto stats = client_->CountItemsByColor(host);
    ASSERT_EQ(stats.size(), 2U);

    EXPECT_EQ(stats[0].color.value_or(Color{}), Color::kRed);
    EXPECT_EQ(stats[0].amount.value_or(0), 2);
    EXPECT_EQ(stats[1].color.value_or(Color{}), Color::kGreen);
    EXPECT_EQ(stats[1].amount.value_or(0), 1);
}

}  // namespace sqldto_tests
