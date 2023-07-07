#include <userver/utils/strong_typedef.hpp>

#include <string>
#include <unordered_map>
#include <variant>

#include <boost/type_traits/has_equal_to.hpp>

#include <userver/logging/log.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

using MyString =
    utils::StrongTypedef<class MyStringTag, std::string,
                         utils::StrongTypedefOps::kCompareTransparent>;
struct MyString2 final
    : utils::StrongTypedef<MyString2, std::string,
                           utils::StrongTypedefOps::kCompareTransparent> {
  using StrongTypedef::StrongTypedef;
};

using MySpecialInt =
    utils::StrongTypedef<class MySpecialIntTag, int,
                         utils::StrongTypedefOps::kCompareTransparent>;

using MySpecialInt2 =
    utils::StrongTypedef<class MySpecialInt2Tag, int,
                         utils::StrongTypedefOps::kCompareTransparent>;
using MySpecialShort =
    utils::StrongTypedef<class MySpecialShortTag, short,
                         utils::StrongTypedefOps::kCompareTransparent>;

using MySpecialVector =
    utils::StrongTypedef<class MySpecialVectorTag, std::vector<bool>>;

struct EmptyStruct {
  static constexpr bool kOk = true;
};

}  // namespace

TEST(StrongTypedef, CompareStrong) {
  struct IntTag {};
  using Int = utils::StrongTypedef<IntTag, int,
                                   utils::StrongTypedefOps::kCompareStrong>;
  EXPECT_TRUE((boost::has_equal_to<Int, Int>::value));
  EXPECT_FALSE((boost::has_equal_to<int, Int>::value));
  EXPECT_FALSE((boost::has_equal_to<Int, int>::value));

  struct StringTag {};
  using String = utils::StrongTypedef<StringTag, std::string,
                                      utils::StrongTypedefOps::kCompareStrong>;
  EXPECT_TRUE((boost::has_equal_to<String, String>::value));
  EXPECT_FALSE((boost::has_equal_to<int, String>::value));
  EXPECT_FALSE((boost::has_equal_to<String, int>::value));
  EXPECT_FALSE((boost::has_equal_to<std::string, String>::value));
  EXPECT_FALSE((boost::has_equal_to<String, std::string>::value));
}

TEST(StrongTypedef, StringDefaultConstruction) {
  EXPECT_EQ("", MyString());
  EXPECT_EQ("", MyString{});
  EXPECT_EQ(MyString(), MyString{});
}

TEST(StrongTypedef, StringTransparentComparison) {
  EXPECT_EQ(MyString("Hello word"), "Hello word");

  MyString str{"word"};
  EXPECT_NE(str, "hello");
  str = {};
  EXPECT_EQ(str, "");

  // EXPECT_NE(str, MyString2{"qwe"}); // as expected: static asserts
}

TEST(StrongTypedef, String2TransparentComparison) {
  MyString2 ms("Hello word");

  EXPECT_EQ(MyString2("Hello word"), "Hello word");

  MyString2 str{"word"};
  EXPECT_NE(str, "hello");
  str = {};
  EXPECT_EQ(str, "");
}

TEST(StrongTypedef, StringStreamingAndLogging) {
  MyString str{"word"};

  std::ostringstream oss;
  oss << str;
  LOG_DEBUG() << str << oss.str();
}

TEST(StrongTypedef, StringInContainer) {
  std::unordered_map<MyString, MyString> umap = {
      {MyString{"Hello"}, MyString{"World"}},
  };
  EXPECT_EQ(umap[MyString{"Hello"}], "World");

  // Fails to compile (as expected):
  // std::unordered_map<std::string, std::string> umap2;
  // umap2[str] = str;
}

TEST(StrongTypedef, IntDefaultConstruction) {
  EXPECT_EQ(0, MySpecialInt());
  EXPECT_EQ(0, MySpecialInt{});
  EXPECT_EQ(MySpecialInt(), MySpecialInt{});
}

TEST(StrongTypedef, IntTransparentComparisons) {
  MySpecialInt i;
  ASSERT_EQ(0, i);

  ++i.GetUnderlying();
  EXPECT_EQ(i, 1);
  EXPECT_LE(i, 1);
  EXPECT_LT(i, 2);
  EXPECT_GT(i, 0);

  EXPECT_EQ(UnderlyingValue(i), 1);
}

TEST(StrongTypedef, IntStreamingAndLogging) {
  MySpecialInt i;
  std::ostringstream oss;
  oss << i;
  LOG_DEBUG() << i << oss.str();
}

TEST(StrongTypedef, UnorderedMap) {
  using MyMap =
      utils::StrongTypedef<class MyMapTag,
                           std::unordered_map<std::string, std::string>>;

  MyMap map = {
      {"Once", "upon a midnight dreary"},
      {"while I pondered", "weak and weary"},
      {"Over many a quaint and curious", "volumes of forgotten lore"},
  };

  EXPECT_EQ(map["Once"], "upon a midnight dreary");
}

TEST(StrongTypedef, UnorderedMapFromStrongTypedefs) {
  using MyMap =
      utils::StrongTypedef<class MyMapTag,
                           std::unordered_map<MyString, MySpecialInt>>;

  MyMap the_rings = {
      {MyString{"Elven-kings"}, MySpecialInt{3}},
      {MyString{"Dwarf-lords"}, MySpecialInt{7}},
      {MyString{"Mortal Men"}, MySpecialInt{9}},
      {MyString{"Dark Lord"}, MySpecialInt{1}},
  };

  --the_rings[MyString{"Dark Lord"}].GetUnderlying();
  EXPECT_EQ(the_rings[MyString{"Dark Lord"}], 0);
  EXPECT_EQ(the_rings[MyString{"Elven-kings"}], 3);
}

TEST(StrongTypedef, Variant) {
  using MyVariant = utils::StrongTypedef<class MyvariantTag,
                                         std::variant<MySpecialInt, MyString>>;

  MyVariant v{MySpecialInt{10}};
  EXPECT_EQ(std::get<MySpecialInt>(v.GetUnderlying()), 10);
}

TEST(StrongTypedef, EmptyStruct) {
  using MyEmptyStruct =
      utils::StrongTypedef<class MyEmptyStructTag, EmptyStruct>;

  MyEmptyStruct v;
  EXPECT_TRUE(v.GetUnderlying().kOk);
}

TEST(StrongTypedef, MyIntId) {
  using MyIntId = utils::StrongTypedef<class MyIntIdTag, int>;

  MyIntId id1{123};
  MyIntId id2{456};

  EXPECT_NE(id1, id2);
  EXPECT_EQ(id1, MyIntId{id1});
}

TEST(StrongTypedef, MyStringId) {
  struct MyStringId final : utils::StrongTypedef<MyStringId, std::string> {
    using StrongTypedef::StrongTypedef;
  };

  MyStringId id1{"123"};
  MyStringId id2{"456"};

  EXPECT_NE(id1, id2);
  EXPECT_EQ(id1, MyStringId{id1});
}

TEST(StrongTypedef, NotConstructible) {
  using utils::impl::strong_typedef::IsStrongToStrongConversion;
  EXPECT_TRUE((IsStrongToStrongConversion<MyString, MyString2>()));
  EXPECT_TRUE((IsStrongToStrongConversion<MyString2, MyString>()));
  EXPECT_FALSE((IsStrongToStrongConversion<MyString, MyString>()));
  EXPECT_FALSE((IsStrongToStrongConversion<MyString2, MyString, MyString>()));
  EXPECT_FALSE((IsStrongToStrongConversion<MyString2, int, int>()));

  EXPECT_TRUE((IsStrongToStrongConversion<MySpecialInt, MySpecialInt2>));
  EXPECT_TRUE((IsStrongToStrongConversion<MySpecialInt2, MySpecialInt>));
  EXPECT_FALSE((std::is_constructible_v<MySpecialInt, MySpecialShort>));
  EXPECT_FALSE((std::is_constructible_v<MySpecialShort, MySpecialInt>));
}

TEST(StrongTypedef, NotConvertibleImplicitly) {
  struct MyStringId final : utils::StrongTypedef<MyStringId, std::string> {
    using StrongTypedef::StrongTypedef;
  };

  using utils::impl::strong_typedef::IsStrongToStrongConversion;
  EXPECT_TRUE((IsStrongToStrongConversion<MyString, MyString2>()));
  EXPECT_TRUE((IsStrongToStrongConversion<MyString2, MyString>()));
  EXPECT_FALSE((std::is_convertible<MySpecialInt, MyString>::value));
  EXPECT_FALSE((std::is_convertible<MyString, MySpecialInt>::value));
  EXPECT_FALSE((std::is_convertible<MyString, int>::value));

  EXPECT_FALSE((std::is_convertible<MySpecialInt2, MySpecialInt>::value));
  EXPECT_FALSE((std::is_convertible<MySpecialInt, MySpecialInt2>::value));
  EXPECT_FALSE((std::is_convertible<MySpecialShort, MySpecialInt>::value));
  EXPECT_FALSE((std::is_convertible<MySpecialInt, MySpecialShort>::value));

  EXPECT_FALSE((std::is_convertible<MyString, std::string>::value));
  EXPECT_FALSE((std::is_convertible<MyString2, std::string>::value));
  EXPECT_FALSE((std::is_convertible<MySpecialInt, int>::value));
  EXPECT_FALSE((std::is_convertible<MyStringId, std::string>::value));
}

TEST(StrongTypedef, NotAssignableImplicitly) {
  struct MyStringId final : utils::StrongTypedef<MyStringId, std::string> {
    using StrongTypedef::StrongTypedef;
  };

  EXPECT_FALSE((std::is_assignable<MyString, MyString2>::value));
  EXPECT_FALSE((std::is_assignable<MyString2, MyString>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialInt, MyString>::value));
  EXPECT_FALSE((std::is_assignable<MyString, MySpecialInt>::value));
  EXPECT_FALSE((std::is_assignable<MyString, int>::value));

  EXPECT_FALSE((std::is_assignable<MyString, std::string>::value));
  EXPECT_FALSE((std::is_assignable<MyString2, std::string>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialInt, int>::value));
  EXPECT_FALSE((std::is_assignable<MyStringId, std::string>::value));

  EXPECT_FALSE((std::is_assignable<MyString&, MyString2>::value));
  EXPECT_FALSE((std::is_assignable<MyString2&, MyString>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialInt&, MyString>::value));
  EXPECT_FALSE((std::is_assignable<MyString&, MySpecialInt>::value));
  EXPECT_FALSE((std::is_assignable<MyString&, int>::value));

  EXPECT_FALSE((std::is_assignable<MyString&, std::string>::value));
  EXPECT_FALSE((std::is_assignable<MyString2&, std::string>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialInt&, int>::value));
  EXPECT_FALSE((std::is_assignable<MyStringId&, std::string>::value));

  EXPECT_FALSE((std::is_assignable<MySpecialInt2, MySpecialInt>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialInt, MySpecialInt2>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialShort, MySpecialInt>::value));
  EXPECT_FALSE((std::is_assignable<MySpecialInt, MySpecialShort>::value));
}

TEST(StrongTypedef, StrongCast) {
  EXPECT_TRUE((utils::IsStrongTypedef<MyString>::value));
  EXPECT_TRUE((utils::IsStrongTypedef<MyString2>::value));
  MyString ms{"string"};
  auto ms2 = utils::StrongCast<MyString2>(ms);
  EXPECT_EQ(ms.GetUnderlying(), ms2.GetUnderlying());
}

TEST(StrongTypedef, StrongTypedefForStringIsNotARange) {
  // Range methods are forwarded
  EXPECT_TRUE(meta::kIsRange<MySpecialVector>);
  EXPECT_FALSE(meta::kIsRange<MySpecialInt>);

  // Except for 'std::string', to avoid giving serialization facilities
  // a potential for seeing "some custom range type" and silently serializing
  // 'std::string' as an array.
  EXPECT_FALSE(meta::kIsRange<MyString>);
  EXPECT_FALSE(meta::kIsRange<MyString2>);
}

USERVER_NAMESPACE_END
