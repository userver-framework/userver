#include <userver/utils/statistics/rate.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/fmt.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

UTEST(Rate, Basic) {
  EXPECT_EQ(Rate{5}, Rate{2} + Rate{3});

  {
    Rate test1{5};
    test1 += Rate{10};
    EXPECT_EQ(Rate{15}, test1);
  }
}

UTEST(Rate, Fmt) { EXPECT_EQ("10", fmt::format("{}", Rate{10})); }

UTEST(Rate, JsonSerialization) {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  builder["test"] = Rate{10};

  const auto val = builder.ExtractValue();

  EXPECT_EQ(10, val["test"].As<int>());
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
