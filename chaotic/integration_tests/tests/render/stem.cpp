#include <gtest/gtest.h>

#include <userver/formats/json/inline.hpp>

#include <stem_schemas/station.internal.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Stem, DottedFileNameIsCppNamespace) {
    const auto json = formats::json::MakeObject("name", "foo");
    const auto spot = json.As<ns::station_internal::Spot>();

    EXPECT_EQ(spot.name, "foo");
    EXPECT_EQ(TestToJsonString(spot), json);
}

USERVER_NAMESPACE_END
