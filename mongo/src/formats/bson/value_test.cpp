#include <gtest/gtest.h>

#include <userver/formats/bson.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/literals.hpp>

USERVER_NAMESPACE_BEGIN

namespace fb = formats::bson;

namespace {

const auto kDoc =
    fb::MakeDoc("arr", fb::MakeArray(1, "elem", fb::MinKey{}),      //
                "doc", fb::MakeDoc("b", true, "i", 0, "d", -1.25),  //
                "null", nullptr,                                    //
                "bool", false);

// Do not ever replicate this in your code.
const auto kDuplicateFieldsDoc = fb::MakeDoc(
    "a", "first", "b", "other", "a", "second", "a", "third", "c", "end");

}  // namespace

static_assert(!std::is_assignable_v<
                  decltype(*std::declval<fb::Value>().begin()), fb::Value>,
              "Value iterators are not assignable");

TEST(BsonValue, SubvalAccess) {
  EXPECT_TRUE(kDoc["missing"].IsMissing());
  EXPECT_TRUE(kDoc["arr"].IsArray());
  UEXPECT_NO_THROW(kDoc["arr"][1]);
  UEXPECT_THROW(kDoc["arr"]["1"], fb::TypeMismatchException);
  EXPECT_TRUE(kDoc["doc"].IsDocument());
  UEXPECT_NO_THROW(kDoc["doc"]["d"]);
  UEXPECT_NO_THROW(kDoc["doc"]["?"]);
  EXPECT_TRUE(kDoc["doc"]["?"].IsMissing());
}

TEST(BsonValue, Array) {
  ASSERT_TRUE(kDoc["arr"].IsArray());
  const auto& arr = kDoc["arr"];
  EXPECT_FALSE(arr.IsEmpty());
  ASSERT_EQ(3, arr.GetSize());

  EXPECT_EQ(1, arr[0].As<int>());
  EXPECT_EQ("elem", arr[1].As<std::string>());
  EXPECT_TRUE(arr[2].IsMinKey());

  UEXPECT_THROW(arr[-1], fb::OutOfBoundsException);
  UEXPECT_THROW(arr[3], fb::OutOfBoundsException);

  int i = 0;
  for (auto it = arr.begin(); it != arr.end(); ++it, ++i) {
    EXPECT_EQ(i, it.GetIndex());
    UEXPECT_THROW(it.GetName(), fb::TypeMismatchException);
  }

  i = 0;
  for (auto it = arr.rbegin(); it != arr.rend(); ++it, ++i) {
    EXPECT_EQ(i, it.GetIndex());
  }

  i = 0;
  for (const auto& elem : arr) {
    switch (i++) {
      case 0:
        EXPECT_EQ(1, elem.As<int>());
        break;
      case 1:
        EXPECT_EQ("elem", elem.As<std::string>());
        break;
      case 2:
        EXPECT_TRUE(elem.IsMinKey());
        break;
      default:
        FAIL() << "Too many elements";
    }
  }

  auto it = arr.rbegin();
  for (std::size_t i = 0; i < arr.GetSize(); ++i) {
    switch (i) {
      case 0:
        EXPECT_TRUE(it->IsMinKey());
        break;
      case 1:
        EXPECT_EQ("elem", it->As<std::string>());
        break;
      case 2:
        EXPECT_EQ(1, it->As<int>());
        break;
      default:
        FAIL() << "Too many elements";
    }
    ++it;
  }
}

TEST(BsonValue, Document) {
  ASSERT_TRUE(kDoc["doc"].IsDocument());

  auto test_doc = [](const fb::Value& doc) {
    EXPECT_TRUE(doc.HasMember("b"));
    EXPECT_TRUE(doc.HasMember("i"));
    EXPECT_TRUE(doc.HasMember("d"));
    EXPECT_FALSE(doc.HasMember("a"));

    EXPECT_TRUE(doc["b"].As<bool>());
    EXPECT_EQ(0, doc["i"].As<int>());
    EXPECT_DOUBLE_EQ(-1.25, doc["d"].As<double>());

    for (auto it = doc.begin(); it != doc.end(); ++it) {
      UEXPECT_THROW(it.GetIndex(), fb::TypeMismatchException);

      if (it.GetName() == "b") {
        EXPECT_TRUE(it->As<bool>());
      } else if (it.GetName() == "i") {
        EXPECT_EQ(0, it->As<int>());
      } else if (it.GetName() == "d") {
        EXPECT_DOUBLE_EQ(-1.25, it->As<double>());
      } else {
        ADD_FAILURE() << "Unexpected key '" << it.GetName() << '\'';
      }
    }
  };
  test_doc(kDoc["doc"]);
  test_doc(kDoc["doc"].As<fb::Document>());
  const auto docref = kDoc["doc"];
  test_doc(kDoc["doc"].As<fb::Document>());
}

TEST(BsonValue, Empty) {
  EXPECT_FALSE(kDoc.IsEmpty());
  EXPECT_FALSE(kDoc["arr"].IsEmpty());
  EXPECT_FALSE(kDoc["doc"].IsEmpty());
  EXPECT_TRUE(kDoc["null"].IsEmpty());
  UEXPECT_THROW(kDoc["bool"].IsEmpty(), fb::TypeMismatchException);
}

TEST(BsonValue, Size) {
  EXPECT_EQ(4, kDoc.GetSize());
  EXPECT_EQ(3, kDoc["arr"].GetSize());
  EXPECT_EQ(3, kDoc["doc"].GetSize());
  EXPECT_EQ(0, kDoc["null"].GetSize());
  UEXPECT_THROW(kDoc["bool"].GetSize(), fb::TypeMismatchException);
}

TEST(BsonValue, Comparison) {
  EXPECT_FALSE(kDoc["arr"] == kDoc);
  EXPECT_TRUE(kDoc["arr"] != kDoc);
  EXPECT_FALSE(kDoc == kDoc["arr"]);
  EXPECT_TRUE(kDoc != kDoc["arr"]);

  EXPECT_FALSE(kDoc["arr"] == kDoc["doc"]);
  EXPECT_TRUE(kDoc["arr"] != kDoc["doc"]);
  EXPECT_FALSE(kDoc["doc"] == kDoc["arr"]);
  EXPECT_TRUE(kDoc["doc"] != kDoc["arr"]);

  EXPECT_FALSE(kDoc["arr"] == kDoc["null"]);
  EXPECT_TRUE(kDoc["arr"] != kDoc["null"]);
  EXPECT_FALSE(kDoc["null"] == kDoc["arr"]);
  EXPECT_TRUE(kDoc["null"] != kDoc["arr"]);

  const auto arr = kDoc["arr"];
  EXPECT_TRUE(kDoc["arr"] == arr);
  EXPECT_FALSE(kDoc["arr"] != arr);
  EXPECT_TRUE(arr == kDoc["arr"]);
  EXPECT_FALSE(arr != kDoc["arr"]);

  const auto doc = kDoc["doc"];
  EXPECT_FALSE(kDoc["arr"] == doc);
  EXPECT_TRUE(kDoc["arr"] != doc);
  EXPECT_FALSE(doc == kDoc["arr"]);
  EXPECT_TRUE(doc != kDoc["arr"]);
}

TEST(BsonValue, Path) {
  EXPECT_EQ("/", kDoc.GetPath());
  EXPECT_EQ("arr", kDoc["arr"].GetPath());
  EXPECT_EQ("arr[0]", kDoc["arr"][0].GetPath());
  EXPECT_EQ("arr[2]", kDoc["arr"][2].GetPath());
  EXPECT_EQ("doc", kDoc["doc"].GetPath());
  EXPECT_EQ("doc.i", kDoc["doc"]["i"].GetPath());
  EXPECT_EQ("doc.b", kDoc["doc"]["b"].GetPath());
  EXPECT_EQ("doc.m", kDoc["doc"]["m"].GetPath());
  EXPECT_EQ("missing", kDoc["missing"].GetPath());
  EXPECT_EQ("missing.sub", kDoc["missing"]["sub"].GetPath());
}

TEST(BsonValue, Null) {
  fb::Value null;
  EXPECT_TRUE(null.IsNull());
  EXPECT_EQ(kDoc["null"], null);
  EXPECT_TRUE(null.IsEmpty());
  EXPECT_EQ(0, null.GetSize());
  EXPECT_EQ(null.begin(), null.end());
  EXPECT_EQ(null.rbegin(), null.rend());
  UEXPECT_NO_THROW(null.HasMember("f"));
  UEXPECT_NO_THROW(null["f"]);
  EXPECT_TRUE(null["f"].IsMissing());
  UEXPECT_THROW(null[0], fb::OutOfBoundsException);
}

TEST(BsonValue, Default) {
  UEXPECT_THROW(kDoc["nonexistent"].As<std::string>(),
                fb::MemberMissingException);
  EXPECT_EQ("no", kDoc["nonexistent"].As<std::string>(std::string("no")));
  EXPECT_EQ("no", kDoc["nonexistent"].As<std::string>("no"));
  EXPECT_EQ("no", kDoc["nonexistent"].As<std::string>("nope", 2));
  UEXPECT_THROW(kDoc["nonexistent"].As<int>(), fb::MemberMissingException);
  EXPECT_EQ(0, kDoc["nonexistent"].As<int>(0));
}

TEST(BsonValue, DuplicateFieldsForbid) {
  auto doc_forbid = kDuplicateFieldsDoc;
  // kForbid is expected to be the default
  UEXPECT_THROW(doc_forbid["a"], fb::ParseException);
}

TEST(BsonValue, DuplicateFieldsUseFirst) {
  auto doc_use_first = kDuplicateFieldsDoc;
  doc_use_first.SetDuplicateFieldsPolicy(
      fb::Value::DuplicateFieldsPolicy::kUseFirst);
  EXPECT_EQ("end", doc_use_first["c"].As<std::string>());
  EXPECT_EQ("first", doc_use_first["a"].As<std::string>());
}

TEST(BsonValue, DuplicateFieldsUseLast) {
  auto doc_use_last = kDuplicateFieldsDoc;
  doc_use_last.SetDuplicateFieldsPolicy(
      fb::Value::DuplicateFieldsPolicy::kUseLast);
  EXPECT_EQ("end", doc_use_last["c"].As<std::string>());
  EXPECT_EQ("third", doc_use_last["a"].As<std::string>());
}

TEST(BsonValue, Items) {
  for ([[maybe_unused]] const auto& [key, value] : Items(kDoc)) {
  }
}

TEST(BsonValue, NullAsDefaulted) {
  auto doc = fb::MakeDoc("nulled", nullptr);

  EXPECT_EQ(doc["nulled"].As<int>({}), 0);
  EXPECT_EQ(doc["nulled"].As<std::vector<int>>({}), std::vector<int>{});

  EXPECT_EQ(doc["nulled"].As<int>(42), 42);

  std::vector<int> value{4, 2};
  EXPECT_EQ(doc["nulled"].template As<std::vector<int>>(value), value);
}

TEST(BsonValue, ExampleUsage) {
  /// [Sample formats::bson::Value usage]
  // #include <userver/formats/bson.hpp>

  auto doc = formats::bson::MakeDoc("key1", 1, "key2",
                                    formats::bson::MakeDoc("key3", "val"));
  formats::bson::Value bson =
      formats::bson::FromBinaryString(ToBinaryString(doc).GetView());

  const auto key1 = bson["key1"].As<int>();
  ASSERT_EQ(key1, 1);

  const auto key3 = bson["key2"]["key3"].As<std::string>();
  ASSERT_EQ(key3, "val");
  /// [Sample formats::bson::Value usage]
}

/// [Sample formats::bson::Value::As<T>() usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};
//  The function must be declared in the namespace of your type
MyKeyValue Parse(const formats::bson::Value& bson,
                 formats::parse::To<MyKeyValue>) {
  return MyKeyValue{
      bson["field1"].As<std::string>(
          {}),                    // default construct string if no value
      bson["field2"].As<int>(1),  // return `1` if "field2" is missing
  };
}

TEST(FormatsBson, ExampleUsageMyStruct) {
  auto doc = formats::bson::MakeDoc(
      "my_value", formats::bson::MakeDoc("field1", "one", "field2", 1));

  formats::bson::Value bson =
      formats::bson::FromBinaryString(ToBinaryString(doc).GetView());
  auto data = bson["my_value"].As<MyKeyValue>();
  EXPECT_EQ(data.field1, "one");
  EXPECT_EQ(data.field2, 1);
}
}  // namespace my_namespace
/// [Sample formats::bson::Value::As<T>() usage]

TEST(FormatsBson, UserDefinedLiterals) {
  using ValueBuilder = formats::bson::ValueBuilder;
  ValueBuilder builder{formats::common::Type::kObject};
  builder["test"] = 3;
  EXPECT_EQ(builder.ExtractValue(),
            R"bson(
    {
      "test": 3
    }
    )bson"_bson);
}

USERVER_NAMESPACE_END
