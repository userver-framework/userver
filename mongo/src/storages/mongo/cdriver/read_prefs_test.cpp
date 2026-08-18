#include <storages/mongo/cdriver/wrappers.hpp>

#include <array>
#include <chrono>
#include <optional>

#include <gtest/gtest.h>
#include <mongoc/mongoc.h>

#include <userver/formats/bson.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

constexpr std::chrono::seconds kDefaultMaxStaleness{90};
constexpr std::chrono::seconds kOperationMaxStaleness{120};

constexpr std::array kModesSupportingMaxStaleness{
    MONGOC_READ_PRIMARY_PREFERRED,
    MONGOC_READ_SECONDARY,
    MONGOC_READ_SECONDARY_PREFERRED,
    MONGOC_READ_NEAREST,
};

TEST(ReadPrefs, AppliesDefaultMaxStalenessToSupportedModes) {
    for (const auto mode : kModesSupportingMaxStaleness) {
        SCOPED_TRACE(mode);
        const ReadPrefsPtr operation_read_prefs{mode};

        const auto
            effective_read_prefs = MakeReadPrefsWithDefaultMaxStaleness(operation_read_prefs, kDefaultMaxStaleness);

        ASSERT_TRUE(effective_read_prefs);
        EXPECT_EQ(mode, mongoc_read_prefs_get_mode(effective_read_prefs.Get()));
        EXPECT_EQ(
            kDefaultMaxStaleness.count(),
            mongoc_read_prefs_get_max_staleness_seconds(effective_read_prefs.Get())
        );
        EXPECT_EQ(MONGOC_NO_MAX_STALENESS, mongoc_read_prefs_get_max_staleness_seconds(operation_read_prefs.Get()));
    }
}

TEST(ReadPrefs, PreservesOperationMaxStaleness) {
    ReadPrefsPtr operation_read_prefs{MONGOC_READ_SECONDARY_PREFERRED};
    mongoc_read_prefs_set_max_staleness_seconds(operation_read_prefs.Get(), kOperationMaxStaleness.count());

    const auto effective_read_prefs = MakeReadPrefsWithDefaultMaxStaleness(operation_read_prefs, kDefaultMaxStaleness);

    ASSERT_TRUE(effective_read_prefs);
    EXPECT_EQ(MONGOC_READ_SECONDARY_PREFERRED, mongoc_read_prefs_get_mode(effective_read_prefs.Get()));
    EXPECT_EQ(kOperationMaxStaleness.count(), mongoc_read_prefs_get_max_staleness_seconds(effective_read_prefs.Get()));
}

TEST(ReadPrefs, PreservesOperationTags) {
    ReadPrefsPtr operation_read_prefs{MONGOC_READ_SECONDARY_PREFERRED};
    const auto tag = formats::bson::MakeDoc("dc", "sas");
    const bson_t* native_tag_bson_ptr = tag.GetBson().get();
    mongoc_read_prefs_add_tag(operation_read_prefs.Get(), native_tag_bson_ptr);

    const auto effective_read_prefs = MakeReadPrefsWithDefaultMaxStaleness(operation_read_prefs, kDefaultMaxStaleness);

    ASSERT_TRUE(effective_read_prefs);
    EXPECT_TRUE(bson_equal(
        mongoc_read_prefs_get_tags(operation_read_prefs.Get()),
        mongoc_read_prefs_get_tags(effective_read_prefs.Get())
    ));
}

TEST(ReadPrefs, DoesNotApplyDefaultMaxStalenessToPrimary) {
    const ReadPrefsPtr operation_read_prefs{MONGOC_READ_PRIMARY};

    const auto effective_read_prefs = MakeReadPrefsWithDefaultMaxStaleness(operation_read_prefs, kDefaultMaxStaleness);

    ASSERT_TRUE(effective_read_prefs);
    EXPECT_EQ(MONGOC_READ_PRIMARY, mongoc_read_prefs_get_mode(effective_read_prefs.Get()));
    EXPECT_EQ(MONGOC_NO_MAX_STALENESS, mongoc_read_prefs_get_max_staleness_seconds(effective_read_prefs.Get()));
}

TEST(ReadPrefs, KeepsMissingOperationReadPrefsMissing) {
    const ReadPrefsPtr operation_read_prefs;

    const auto effective_read_prefs = MakeReadPrefsWithDefaultMaxStaleness(operation_read_prefs, kDefaultMaxStaleness);

    EXPECT_FALSE(effective_read_prefs);
}

TEST(ReadPrefs, KeepsMaxStalenessMissingWithoutDefault) {
    const ReadPrefsPtr operation_read_prefs{MONGOC_READ_SECONDARY_PREFERRED};

    const auto effective_read_prefs = MakeReadPrefsWithDefaultMaxStaleness(operation_read_prefs, std::nullopt);

    ASSERT_TRUE(effective_read_prefs);
    EXPECT_NE(operation_read_prefs.Get(), effective_read_prefs.Get());
    EXPECT_EQ(MONGOC_READ_SECONDARY_PREFERRED, mongoc_read_prefs_get_mode(effective_read_prefs.Get()));
    EXPECT_EQ(MONGOC_NO_MAX_STALENESS, mongoc_read_prefs_get_max_staleness_seconds(effective_read_prefs.Get()));
}

}  // namespace
}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
