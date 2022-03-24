#include <userver/yaml_config/yaml_config.hpp>

#include <gtest/gtest.h>

#include <formats/common/value_test.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct Parsing<yaml_config::YamlConfig> : public ::testing::Test {
  constexpr static auto FromString = [](const std::string& str) {
    return yaml_config::YamlConfig{formats::yaml::FromString(str), {}};
  };
  using ParseException = yaml_config::ParseException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(YamlConfig, Parsing, yaml_config::YamlConfig);

TEST(YamlConfig, Basic) {
  auto node = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
  )");

  yaml_config::YamlConfig conf(std::move(node), {});
  EXPECT_EQ(conf["string"].As<std::string>(), "hello");
  EXPECT_EQ(conf["duration1"].As<std::chrono::milliseconds>(),
            std::chrono::seconds(10));
  EXPECT_EQ(conf["duration2"].As<std::chrono::milliseconds>(),
            std::chrono::milliseconds(10));
  EXPECT_EQ(conf["duration3"].As<std::chrono::milliseconds>(),
            std::chrono::seconds(1));
  EXPECT_EQ(conf["int"].As<int>(), 42);
}

TEST(YamlConfig, VariableMap) {
  auto vmap = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
    elem0: str123
    not_missing: xxx
  )");

  auto node = formats::yaml::FromString(R"(
    string: $string
    duration1: $duration1
    duration2: $duration2
    duration3: $duration3
    int: $int
    array:
      - $elem0
      - str222
    array_with_missing:
      - $missing_elem
      - str333
      - $not_missing
      - $missing_elem2
    bad_array:
      - str
      - arr:
        - aaa
        - bbb
  )");

  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));
  EXPECT_EQ(conf["string"].As<std::string>(), "hello");
  EXPECT_EQ(conf["duration1"].As<std::chrono::milliseconds>(),
            std::chrono::seconds(10));
  EXPECT_EQ(conf["duration2"].As<std::chrono::milliseconds>(),
            std::chrono::milliseconds(10));
  EXPECT_EQ(conf["duration3"].As<std::chrono::milliseconds>(),
            std::chrono::seconds(1));
  EXPECT_EQ(conf["int"].As<int>(), 42);
  std::vector<std::string> expected_vec{"str123", "str222"};
  EXPECT_EQ(conf["array"].As<std::vector<std::string>>(), expected_vec);
  UEXPECT_THROW(conf["bad_array"].As<std::vector<std::string>>(),
                yaml_config::Exception);

  std::vector<std::optional<std::string>> expected_vec_miss{
      std::nullopt, std::optional<std::string>("str333"),
      std::optional<std::string>("xxx"), std::nullopt};
  EXPECT_EQ(
      conf["array_with_missing"].As<std::vector<std::optional<std::string>>>(),
      expected_vec_miss);
}

/// Common test base to make consistent tests between iterators and operator[]
template <typename Accessor>
class YamlConfigAccessor : public ::testing::Test {};

struct SquareBracketAccessor {
  explicit SquareBracketAccessor(const yaml_config::YamlConfig& value_)
      : value(value_) {}

  template <typename X>
  auto Access(X&& arg) const {
    return value[std::forward<X>(arg)];
  }

  auto operator[](const std::string& key) const { return Access(key); }
  auto operator[](size_t index) const { return Access(index); }

  const yaml_config::YamlConfig& value;
};

struct IteratorAccessor {
  explicit IteratorAccessor(const yaml_config::YamlConfig& value_)
      : value(value_) {}

  auto Access(size_t index) const {
    auto it = value.begin();
    auto eit = value.end();
    if (it.GetIteratorType() != formats::common::Type::kArray) {
      ADD_FAILURE() << "Can't use indices with iterators over objects";
      return yaml_config::YamlConfig({}, {});
    }
    std::advance(it, static_cast<decltype(it)::difference_type>(index));
    return *it;
  };

  auto Access(const std::string& key) const {
    auto it = value.begin();
    auto eit = value.end();
    // let's cheat a bit
    if (it.GetIteratorType() != formats::common::Type::kObject) {
      ADD_FAILURE() << "Can't use string keys with iterator over array";
    }

    while (it != eit && it.GetName() != key) {
      it++;
    }
    if (it != eit) {
      return *it;
    } else {
      ADD_FAILURE() << "Iterator went out of range";
      return yaml_config::YamlConfig({}, {});
    }
  }

  auto operator[](const std::string& key) const { return Access(key); }
  auto operator[](size_t index) const { return Access(index); }

  const yaml_config::YamlConfig& value;
};

using YamlConfigAccessorTypes =
    ::testing::Types<SquareBracketAccessor, IteratorAccessor>;

TYPED_TEST_SUITE(YamlConfigAccessor, YamlConfigAccessorTypes);

TYPED_TEST(YamlConfigAccessor, SquareBracketOperatorArray) {
  using Accessor = TypeParam;
  auto vmap = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
    test2:
      zzz: asd
    test4: qwe
  )");

  auto node = formats::yaml::FromString(R"(
    root:
      -  member1:  $int
         duration: $duration1
      -  member2:  $string
         duration: $duration2
         miss_val: $miss_val
      - $test2
      - $test3
      - $test4
      - str5
  )");
  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));

  yaml_config::YamlConfig root_array_data = conf["root"];
  Accessor root_array(root_array_data);
  EXPECT_TRUE(root_array_data.Yaml().IsArray());

  EXPECT_EQ(root_array[0]["member1"].template As<int>(), 42);
  EXPECT_EQ(root_array[0]["duration"].template As<std::chrono::milliseconds>(),
            std::chrono::seconds{10});

  EXPECT_EQ(root_array[1]["member2"].template As<std::string>(), "hello");
  EXPECT_EQ(root_array[1]["duration"].template As<std::chrono::milliseconds>(),
            std::chrono::milliseconds{10});
  EXPECT_TRUE(root_array[1]["miss_val"].IsMissing());
  EXPECT_TRUE(root_array[1]["miss_val2"].IsMissing());
  UEXPECT_THROW(root_array[1]["miss_val"].template As<std::string>(),
                yaml_config::Exception);

  EXPECT_TRUE(root_array[1]["miss_val"]["sub_val"].IsMissing());
  EXPECT_TRUE(root_array[1]["miss_val2"]["sub_val"].IsMissing());
  EXPECT_EQ(root_array[1]["miss_val"]["sub_val"]
                .template As<std::optional<std::string>>(),
            std::optional<std::string>{});
  EXPECT_EQ(root_array[1]["miss_val2"]["sub_val"]
                .template As<std::optional<std::string>>(),
            std::optional<std::string>{});
  EXPECT_EQ(root_array[1]["miss_val"]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  EXPECT_EQ(root_array[1]["miss_val2"]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  EXPECT_TRUE(root_array[1]["miss_val"][11].IsMissing());
  EXPECT_TRUE(root_array[1]["miss_val2"][11].IsMissing());
  EXPECT_EQ(root_array[1]["miss_val"][11]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  EXPECT_EQ(root_array[1]["miss_val2"][11]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  UEXPECT_THROW(
      root_array[1]["miss_val"][11]["sub_val"].template As<std::string>(),
      yaml_config::Exception);
  UEXPECT_THROW(
      root_array[1]["miss_val2"][11]["sub_val"].template As<std::string>(),
      yaml_config::Exception);

  EXPECT_EQ(root_array[2]["zzz"].template As<std::string>(), "asd");

  EXPECT_TRUE(root_array[3].IsMissing());
  UEXPECT_THROW(root_array[3].template As<std::string>(),
                yaml_config::Exception);
  EXPECT_EQ(root_array[3].template As<std::string>("dflt"), "dflt");

  EXPECT_EQ(root_array[4].template As<std::string>(), "qwe");
  EXPECT_EQ(root_array[5].template As<std::string>(), "str5");

  UEXPECT_THROW(root_array[99]["zzz"].template As<std::string>(),
                yaml_config::Exception);
}

TYPED_TEST(YamlConfigAccessor, SquareBracketOperatorObject) {
  using Accessor = TypeParam;
  auto vmap = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
    test2:
      zzz: asd
    test4: qwe
  )");

  auto node = formats::yaml::FromString(R"(
    root:
      e0:
         member1:  $int
         duration: $duration1
      e1:
         member2:  $string
         duration: $duration2
         miss_val: $miss_val
      e2: $test2
      e3: $test3
      e4: $test4
      e5: str5
  )");
  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));

  yaml_config::YamlConfig root_object_data = conf["root"];
  Accessor root_object(root_object_data);
  EXPECT_TRUE(root_object_data.Yaml().IsObject());

  EXPECT_EQ(root_object["e0"]["member1"].template As<int>(), 42);
  EXPECT_EQ(
      root_object["e0"]["duration"].template As<std::chrono::milliseconds>(),
      std::chrono::seconds{10});

  EXPECT_EQ(root_object["e1"]["member2"].template As<std::string>(), "hello");
  EXPECT_EQ(
      root_object["e1"]["duration"].template As<std::chrono::milliseconds>(),
      std::chrono::milliseconds{10});
  EXPECT_TRUE(root_object["e1"]["miss_val"].IsMissing());
  EXPECT_TRUE(root_object["e1"]["miss_val2"].IsMissing());
  UEXPECT_THROW(root_object["e1"]["miss_val"].template As<std::string>(),
                yaml_config::Exception);

  EXPECT_TRUE(root_object["e1"]["miss_val"]["sub_val"].IsMissing());
  EXPECT_TRUE(root_object["e1"]["miss_val2"]["sub_val"].IsMissing());
  EXPECT_EQ(root_object["e1"]["miss_val"]["sub_val"]
                .template As<std::optional<std::string>>(),
            std::optional<std::string>{});
  EXPECT_EQ(root_object["e1"]["miss_val2"]["sub_val"]
                .template As<std::optional<std::string>>(),
            std::optional<std::string>{});
  EXPECT_EQ(root_object["e1"]["miss_val"]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  EXPECT_EQ(root_object["e1"]["miss_val2"]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  EXPECT_TRUE(root_object["e1"]["miss_val"][11].IsMissing());
  EXPECT_TRUE(root_object["e1"]["miss_val2"][11].IsMissing());
  EXPECT_EQ(root_object["e1"]["miss_val"][11]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  EXPECT_EQ(root_object["e1"]["miss_val2"][11]["sub_val"]
                .template As<std::optional<std::string>>("def"),
            "def");
  UEXPECT_THROW(
      root_object["e1"]["miss_val"][11]["sub_val"].template As<std::string>(),
      yaml_config::Exception);
  UEXPECT_THROW(
      root_object["e1"]["miss_val2"][11]["sub_val"].template As<std::string>(),
      yaml_config::Exception);

  EXPECT_EQ(root_object["e2"]["zzz"].template As<std::string>(), "asd");

  EXPECT_TRUE(root_object["e3"].IsMissing());
  UEXPECT_THROW(root_object["e3"].template As<std::string>(),
                yaml_config::Exception);
  EXPECT_EQ(root_object["e3"].template As<std::string>("dflt"), "dflt");

  EXPECT_EQ(root_object["e4"].template As<std::string>(), "qwe");
  EXPECT_EQ(root_object["e5"].template As<std::string>(), "str5");
}

TEST(YamlConfig, PathsParseInto) {
  auto vmap = formats::yaml::FromString(R"(
    path_vm: /hello/1
    path_array_vm: ["/hello/1", "/path/2"]
  )");

  formats::yaml::Value node = formats::yaml::FromString(R"(
    path: /hello/1
    path_array: ["/hello/1", "/path/2"]
    path_vm: $path_vm
    path_array_vm: $path_array_vm
  )");

  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));

  auto path = conf["path"].As<std::string>();
  EXPECT_EQ(path, "/hello/1");

  auto path_vm = conf["path_vm"].As<std::string>();
  EXPECT_EQ(path_vm, "/hello/1");

  auto path_array = conf["path_array"].As<std::vector<std::string>>();
  ASSERT_EQ(path_array.size(), 2);
  EXPECT_EQ(path_array[0], "/hello/1");
  EXPECT_EQ(path_array[1], "/path/2");

  auto path_array_vm = conf["path_array_vm"].As<std::vector<std::string>>();
  ASSERT_EQ(path_array_vm.size(), 2);
  EXPECT_EQ(path_array_vm[0], "/hello/1");
  EXPECT_EQ(path_array_vm[1], "/path/2");
}

TEST(YamlConfig, Subconfig) {
  auto vmap = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
  )");

  auto node = formats::yaml::FromString(R"(
    member: $string
    subconfig:
      duration1: $duration1
      duration2: $duration2
  )");

  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));
  EXPECT_EQ(conf["member"].As<std::string>(), "hello");
  // Get subconfig
  auto subconf = conf["subconfig"];
  EXPECT_EQ(subconf["duration1"].As<std::chrono::milliseconds>(),
            std::chrono::seconds(10));
  EXPECT_EQ(subconf["duration2"].As<std::chrono::milliseconds>(),
            std::chrono::milliseconds(10));

  // Check missing
  auto missing_subconf = conf["missing"];
  EXPECT_TRUE(missing_subconf.IsMissing());
}

TEST(YamlConfig, SubconfigNotObject) {
  auto vmap = formats::yaml::ValueBuilder(formats::common::Type::kObject)
                  .ExtractValue();

  auto node = formats::yaml::FromString(R"(
    member: hello
  )");

  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));
  EXPECT_EQ(conf["member"].As<std::string>(), "hello");
  EXPECT_TRUE(conf.Yaml().IsObject());
  // Get subconfig that is not an object
  auto subconf = conf["member"];
  // It must throw an exception
  UEXPECT_THROW(subconf["another"], yaml_config::Exception);
}

TEST(YamlConfig, IteratorArray) {
  auto vmap = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
  )");

  auto node = formats::yaml::FromString(R"(
    root:
      -  member1:  $int
         duration: $duration1
      -  member2:  $string
         duration: $duration2
  )");
  yaml_config::YamlConfig conf(std::move(node), std::move(vmap));

  auto root_array = conf["root"];
  EXPECT_TRUE(root_array.Yaml().IsArray());

  // Testing basics
  auto it = root_array.begin();
  auto eit = root_array.end();

  EXPECT_EQ(it, it);
  EXPECT_EQ(eit, eit);
  EXPECT_NE(it, eit);
  EXPECT_NE(eit, it);

  // Test copying
  auto cit = it;
  EXPECT_EQ(it, cit);

  // Test move
  auto cit2 = it;
  auto mit = std::move(cit2);
  EXPECT_EQ(it, mit);
  EXPECT_NE(mit, eit);

  // There are some few simple tests for substitutions. Main tests are done
  // in SquareBracketOperator
  EXPECT_EQ(42, (*it)["member1"].As<int>());
  EXPECT_EQ(42, (*mit)["member1"].As<int>());
  EXPECT_EQ(42, (*cit)["member1"].As<int>());
  EXPECT_EQ(std::chrono::seconds{10},
            (*it)["duration"].As<std::chrono::milliseconds>());

  it++;
  ASSERT_NE(it, eit);
  EXPECT_NE(it, cit);
  EXPECT_NE(it, mit);
  EXPECT_EQ("hello", (*it)["member2"].As<std::string>());
  EXPECT_EQ(std::chrono::milliseconds{10},
            (*it)["duration"].As<std::chrono::milliseconds>());

  it++;
  EXPECT_EQ(it, eit);
  EXPECT_NE(cit, it);
}

USERVER_NAMESPACE_END
