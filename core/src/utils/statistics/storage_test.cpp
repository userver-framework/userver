#include <userver/utils/statistics/storage.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(StatisticsStorage, DotsInPathSegments) {
  utils::statistics::Storage statistics_storage;
  auto statistics_holder = statistics_storage.RegisterExtender(
      {"foo.bar", "baz", "", "a.b.c"},
      [](const auto& /*prefix*/) { return formats::json::ValueBuilder{42}; });

  const auto json = statistics_storage.GetAsJson({""}).ExtractValue();
  EXPECT_EQ(json["foo.bar"]["baz"][""]["a.b.c"].As<int>(), 42);
}

USERVER_NAMESPACE_END
