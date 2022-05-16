#include <gtest/gtest.h>

#include <chrono>

#include <boost/optional.hpp>

#include <userver/formats/bson.hpp>
#include <userver/formats/parse/boost_optional.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace fb = formats::bson;

TEST(BsonConversion, Missing) {
  auto test_elem = [](const fb::Value& elem) {
    EXPECT_TRUE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_FALSE(elem.ConvertTo<bool>());
    EXPECT_EQ(0, elem.ConvertTo<int32_t>());
    EXPECT_EQ(0, elem.ConvertTo<int64_t>());
    EXPECT_EQ(0, elem.ConvertTo<size_t>());
    EXPECT_DOUBLE_EQ(0.0, elem.ConvertTo<double>());
    EXPECT_TRUE(elem.ConvertTo<std::string>().empty());

    EXPECT_EQ(true, elem.ConvertTo<bool>(true));
    EXPECT_EQ(1, elem.ConvertTo<int>(1));
    EXPECT_EQ("test", elem.ConvertTo<std::string>("test"));
    EXPECT_EQ("test", elem.ConvertTo<std::string>("test123", 4));
  };

  const auto doc = fb::MakeDoc("a", fb::MakeArray(), "b", fb::MakeDoc());
  UEXPECT_THROW(doc["a"][0], fb::OutOfBoundsException);
  UEXPECT_THROW(doc["a"]["b"], fb::TypeMismatchException);
  UEXPECT_THROW(doc["b"][0], fb::TypeMismatchException);
  test_elem(doc["b"]["c"]);
  test_elem(doc["d"]);
}

TEST(BsonConversion, Null) {
  auto test_elem = [](const fb::Value& elem) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_TRUE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_FALSE(elem.ConvertTo<bool>());
    EXPECT_EQ(0, elem.ConvertTo<int32_t>());
    EXPECT_EQ(0, elem.ConvertTo<int64_t>());
    EXPECT_EQ(0, elem.ConvertTo<size_t>());
    EXPECT_DOUBLE_EQ(0.0, elem.ConvertTo<double>());
    EXPECT_TRUE(elem.ConvertTo<std::string>().empty());
  };

  const auto doc = fb::MakeDoc("a", fb::MakeArray(nullptr), "e", nullptr);
  test_elem(doc["a"][0]);
  test_elem(doc["e"]);
}

TEST(BsonConversion, Bool) {
  auto test_elem = [](const fb::Value& elem, bool value) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_TRUE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_EQ(value, elem.ConvertTo<bool>());
    EXPECT_EQ(value, elem.ConvertTo<int32_t>());
    EXPECT_EQ(value, elem.ConvertTo<int64_t>());
    EXPECT_EQ(value, elem.ConvertTo<size_t>());
    EXPECT_DOUBLE_EQ(value, elem.ConvertTo<double>());
    if (value) {
      EXPECT_EQ("true", elem.ConvertTo<std::string>());
    } else {
      EXPECT_EQ("false", elem.ConvertTo<std::string>());
    }
  };

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray(false, true), "ef", false, "et", true);
  test_elem(doc["a"][0], false);
  test_elem(doc["a"][1], true);
  test_elem(doc["ef"], false);
  test_elem(doc["et"], true);
}

TEST(BsonConversion, Double) {
  auto test_elem = [](const fb::Value& elem, double value) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_TRUE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_EQ(!!value, elem.ConvertTo<bool>());
    EXPECT_EQ(static_cast<int32_t>(value), elem.ConvertTo<int32_t>());
    EXPECT_EQ(static_cast<int64_t>(value), elem.ConvertTo<int64_t>());
    if (value > -1.0) {
      EXPECT_EQ(static_cast<size_t>(value), elem.ConvertTo<size_t>());
    } else {
      EXPECT_ANY_THROW(elem.ConvertTo<size_t>());
    }
    EXPECT_DOUBLE_EQ(value, elem.ConvertTo<double>());
    EXPECT_EQ(std::to_string(value), elem.ConvertTo<std::string>());
  };

  const auto doc = fb::MakeDoc("a", fb::MakeArray(0.0, 0.123, -0.123), "ez",
                               0.0, "ep", 3.21, "en", -3.21);
  test_elem(doc["a"][0], 0.0);
  test_elem(doc["a"][1], 0.123);
  test_elem(doc["a"][2], -0.123);
  test_elem(doc["ez"], 0.0);
  test_elem(doc["ep"], 3.21);
  test_elem(doc["en"], -3.21);
}

TEST(BsonConversion, Int32) {
  auto test_elem = [](const fb::Value& elem, int32_t value) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_TRUE(elem.IsInt32());
    EXPECT_TRUE(elem.IsInt64());
    EXPECT_TRUE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_EQ(!!value, elem.ConvertTo<bool>());
    EXPECT_EQ(value, elem.ConvertTo<int32_t>());
    EXPECT_EQ(value, elem.ConvertTo<int64_t>());
    if (value >= 0) {
      EXPECT_EQ(value, elem.ConvertTo<size_t>());
    } else {
      EXPECT_ANY_THROW(elem.ConvertTo<size_t>());
    }
    EXPECT_DOUBLE_EQ(value, elem.ConvertTo<double>());
    EXPECT_EQ(std::to_string(value), elem.ConvertTo<std::string>());
  };

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray(int32_t{0}, int32_t{123}, int32_t{-123}),
                  "ez", int32_t{0}, "ep", int32_t{321}, "en", int32_t{-321});
  test_elem(doc["a"][0], 0);
  test_elem(doc["a"][1], 123);
  test_elem(doc["a"][2], -123);
  test_elem(doc["ez"], 0);
  test_elem(doc["ep"], 321);
  test_elem(doc["en"], -321);
}

TEST(BsonConversion, Int64) {
  auto test_elem = [](const fb::Value& elem, int64_t value) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_TRUE(elem.IsInt64());
    EXPECT_TRUE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_EQ(!!value, elem.ConvertTo<bool>());
    EXPECT_EQ(value, elem.ConvertTo<int32_t>());
    EXPECT_EQ(value, elem.ConvertTo<int64_t>());
    if (value >= 0) {
      EXPECT_EQ(value, elem.ConvertTo<size_t>());
    } else {
      EXPECT_ANY_THROW(elem.ConvertTo<size_t>());
    }
    EXPECT_DOUBLE_EQ(value, elem.ConvertTo<double>());
    EXPECT_EQ(std::to_string(value), elem.ConvertTo<std::string>());
  };

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray(int64_t{0}, int64_t{123}, int64_t{-123}),
                  "ez", int64_t{0}, "ep", int64_t{321}, "en", int64_t{-321});
  test_elem(doc["a"][0], 0);
  test_elem(doc["a"][1], 123);
  test_elem(doc["a"][2], -123);
  test_elem(doc["ez"], 0);
  test_elem(doc["ep"], 321);
  test_elem(doc["en"], -321);
}

TEST(BsonConversion, Utf8) {
  auto test_elem = [](const fb::Value& elem, const std::string& value) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_TRUE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_EQ(!value.empty(), elem.ConvertTo<bool>());
    UEXPECT_THROW(elem.ConvertTo<int32_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<int64_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<size_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<double>(), fb::TypeMismatchException);
    EXPECT_EQ(value, elem.ConvertTo<std::string>());
  };

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray("", "test"), "ee", "", "e", "string");
  test_elem(doc["a"][0], {});
  test_elem(doc["a"][1], "test");
  test_elem(doc["ee"], {});
  test_elem(doc["e"], "string");
}

TEST(BsonConversion, Binary) {
  auto test_elem = [](const fb::Value& elem, const std::string& value) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_TRUE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    EXPECT_EQ(!value.empty(), elem.ConvertTo<bool>());
    UEXPECT_THROW(elem.ConvertTo<int32_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<int64_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<size_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<double>(), fb::TypeMismatchException);
    EXPECT_EQ(value, elem.ConvertTo<std::string>());
  };

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray(fb::Binary(""), fb::Binary("test")), "ee",
                  fb::Binary(""), "e", fb::Binary("\377\377"));
  test_elem(doc["a"][0], {});
  test_elem(doc["a"][1], "test");
  test_elem(doc["ee"], {});
  test_elem(doc["e"], "\377\377");
}

TEST(BsonConversion, DateTime) {
  auto test_elem = [](const fb::Value& elem,
                      const std::chrono::system_clock::time_point& value,
                      bool is_int32) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_TRUE(elem.IsDateTime());
    EXPECT_FALSE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    const auto int_value =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            value.time_since_epoch())
            .count();

    EXPECT_TRUE(elem.ConvertTo<bool>());
    if (is_int32) {
      EXPECT_EQ(int_value, elem.ConvertTo<int32_t>());
    } else {
      UEXPECT_THROW(elem.ConvertTo<int32_t>(), fb::ConversionException);
    }
    EXPECT_EQ(int_value, elem.ConvertTo<int64_t>());
    if (sizeof(size_t) >= 8 || is_int32) {
      EXPECT_EQ(int_value, elem.ConvertTo<size_t>());
    } else {
      UEXPECT_THROW(elem.ConvertTo<size_t>(), fb::ConversionException);
    }
    EXPECT_DOUBLE_EQ(int_value, elem.ConvertTo<double>());
    EXPECT_EQ(std::to_string(int_value), elem.ConvertTo<std::string>());
  };

  const auto time1 = std::chrono::system_clock::from_time_t(1535749200);
  const auto time2 = std::chrono::system_clock::from_time_t(1538341200);
  const auto time3 = std::chrono::system_clock::from_time_t(1538341);

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray(time1), "e", time2, "s", time3);
  test_elem(doc["a"][0], time1, false);
  test_elem(doc["e"], time2, false);
  test_elem(doc["s"], time3, true);
}

TEST(BsonConversion, Oid) {
  auto test_elem = [](const fb::Value& elem, const std::string& oid_string) {
    EXPECT_FALSE(elem.IsMissing());
    EXPECT_FALSE(elem.IsArray());
    EXPECT_FALSE(elem.IsDocument());
    EXPECT_FALSE(elem.IsNull());
    EXPECT_FALSE(elem.IsBool());
    EXPECT_FALSE(elem.IsInt32());
    EXPECT_FALSE(elem.IsInt64());
    EXPECT_FALSE(elem.IsDouble());
    EXPECT_FALSE(elem.IsString());
    EXPECT_FALSE(elem.IsDateTime());
    EXPECT_TRUE(elem.IsOid());
    EXPECT_FALSE(elem.IsBinary());
    EXPECT_FALSE(elem.IsDecimal128());
    EXPECT_FALSE(elem.IsMinKey());
    EXPECT_FALSE(elem.IsMaxKey());
    EXPECT_FALSE(elem.IsObject());

    UEXPECT_THROW(elem.ConvertTo<bool>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<int32_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<int64_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<size_t>(), fb::TypeMismatchException);
    UEXPECT_THROW(elem.ConvertTo<double>(), fb::TypeMismatchException);
    EXPECT_EQ(oid_string, elem.ConvertTo<std::string>());
  };

  const auto doc =
      fb::MakeDoc("a", fb::MakeArray(fb::Oid("5b89ac509ecb0a21a6000001")), "e",
                  fb::Oid("5bb139509ecb0a21a6000002"));
  test_elem(doc["a"][0], "5b89ac509ecb0a21a6000001");
  test_elem(doc["e"], "5bb139509ecb0a21a6000002");
}

TEST(BsonConversion, Containers) {
  const auto doc = fb::MakeDoc("a", fb::MakeArray(0, 1, 2), "d",
                               fb::MakeDoc("one", 1.0, "two", 2), "n", nullptr);

  UEXPECT_THROW((doc["a"].ConvertTo<std::unordered_map<std::string, int>>()),
                fb::TypeMismatchException);
  UEXPECT_THROW(doc["d"].ConvertTo<std::vector<int>>(),
                fb::TypeMismatchException);

  auto arr = doc["a"].ConvertTo<std::vector<int>>();
  for (int i = 0; i < static_cast<int>(arr.size()); ++i) {
    EXPECT_EQ(i, arr[i]) << "mismatch at position " << i;
  }

  auto umap = doc["d"].ConvertTo<std::unordered_map<std::string, int>>();
  EXPECT_EQ(1, umap["one"]);
  EXPECT_EQ(2, umap["two"]);

  EXPECT_TRUE(
      (doc["n"].ConvertTo<std::unordered_map<std::string, int>>().empty()));
  EXPECT_TRUE(doc["n"].ConvertTo<std::vector<int>>().empty());
  EXPECT_FALSE(doc["n"].ConvertTo<boost::optional<std::string>>());
  EXPECT_FALSE(doc["n"].ConvertTo<std::optional<std::string>>());
}

USERVER_NAMESPACE_END
