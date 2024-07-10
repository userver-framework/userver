#include <userver/yaml_config/yaml_config.hpp>

#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <formats/common/value_test.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
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

TEST(YamlConfig, SampleVars) {
  /// [sample vars]
  auto node = formats::yaml::FromString(R"(
    some_element:
        some: $variable
  )");
  auto vars = formats::yaml::FromString("variable: 42");

  yaml_config::YamlConfig yaml(std::move(node), vars);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);
  /// [sample vars]
}

TEST(YamlConfig, SampleVarsFallback) {
  auto node = formats::yaml::FromString(R"(
    some_element:
        some: $variable
        some#fallback: 42
  )");

  yaml_config::YamlConfig yaml(std::move(node), {});
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);
}

TEST(YamlConfig, SampleVarsFallbackOnly) {
  auto node = formats::yaml::FromString(R"(
    some_element:
        some#fallback: 42
  )");

  yaml_config::YamlConfig yaml(std::move(node), {});
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);
}

TEST(YamlConfig, SampleEnv) {
  /// [sample env]
  auto node = formats::yaml::FromString(R"(
    some_element:
        some#env: ENV_VARIABLE_NAME
  )");

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::setenv("ENV_VARIABLE_NAME", "100", 1);

  yaml_config::YamlConfig yaml(std::move(node), {},
                               yaml_config::YamlConfig::Mode::kEnvAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 100);
  /// [sample env]
}

TEST(YamlConfig, SampleMultiple) {
  const auto node = formats::yaml::FromString(R"(
# /// [sample multiple]
# yaml
some_element:
    some: $variable
    some#file: /some/path/to/the/file.yaml
    some#env: SOME_ENV_VARIABLE
    some#fallback: 100500
# /// [sample multiple]
  )");
  const auto vars = formats::yaml::FromString("variable: 42");

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::setenv("SOME_ENV_VARIABLE", "100", 1);

  yaml_config::YamlConfig yaml(node, vars);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);

  yaml = yaml_config::YamlConfig(node, vars,
                                 yaml_config::YamlConfig::Mode::kEnvAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);

  yaml = yaml_config::YamlConfig(
      node, vars, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);

  yaml = yaml_config::YamlConfig(node, {},
                                 yaml_config::YamlConfig::Mode::kEnvAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 100);

  yaml = yaml_config::YamlConfig(node, {});
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::unsetenv("SOME_ENV_VARIABLE");

  yaml = yaml_config::YamlConfig(node, {},
                                 yaml_config::YamlConfig::Mode::kEnvAllowed);
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);

  yaml = yaml_config::YamlConfig(
      node, {}, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 100500);
}

TEST(YamlConfig, IterationSkipInternalFields) {
  const auto node = formats::yaml::FromString(R"(
element1:
    some0: 0
    some1: $variable
    some1#file: /some/path/to/the/file.yaml
    some1#env: SOME_ENV_VARIABLE
    some1#fallback: 100500
    some2: 0
element2:
    some#file: /some/path/to/the/file.yaml
    some#env: SOME_ENV_VARIABLE
    some#fallback: 100500
  )");

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::setenv("SOME_ENV_VARIABLE", "100", 1);

  const auto vars = formats::yaml::FromString("variable: 0");

  yaml_config::YamlConfig yaml(
      node, vars, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);

  std::size_t count = 0;
  for (auto [key, value] : Items(yaml["element1"])) {
    ++count;
    EXPECT_THAT(key, testing::Not(testing::HasSubstr("#")));
    EXPECT_EQ(value.As<int>(), 0);
  }
  ASSERT_EQ(count, 3);

  count = 0;
  for (const auto& value : yaml["element1"]) {
    ++count;
    EXPECT_EQ(value.As<int>(), 0);
  }
  ASSERT_EQ(count, 3);

  count = 0;
  auto element1 = yaml["element1"];
  // Testing suffix increment
  for (auto it = element1.begin(); it != element1.end(); it++) {
    ++count;
    EXPECT_THAT(it.GetName(), testing::Not(testing::HasSubstr("#")));
    EXPECT_EQ(it->As<int>(), 0);
  }
  ASSERT_EQ(count, 3);

  count = 0;
  for (auto [key, value] : Items(yaml["element2"])) {
    ++count;
    EXPECT_THAT(key, testing::Not(testing::HasSubstr("#")));
    EXPECT_EQ(value.As<int>(), 100);
  }
  ASSERT_EQ(count, 1);

  count = 0;
  for (const auto& value : yaml["element2"]) {
    ++count;
    EXPECT_EQ(value.As<int>(), 100);
  }
  ASSERT_EQ(count, 1);

  count = 0;
  auto element2 = yaml["element2"];
  // Testing suffix increment
  for (auto it = element2.begin(); it != element2.end(); it++) {
    ++count;
    EXPECT_THAT(it.GetName(), testing::Not(testing::HasSubstr("#")));
    EXPECT_EQ(it->As<int>(), 100);
  }
  ASSERT_EQ(count, 1);
}

TEST(YamlConfig, SampleEnvFallback) {
  auto node = formats::yaml::FromString(R"(
# /// [sample env fallback]
# yaml
some_element:
    some#env: ENV_NAME
    some#fallback: 5
# /// [sample env fallback]
  )");

  yaml_config::YamlConfig yaml(std::move(node), {},
                               yaml_config::YamlConfig::Mode::kEnvAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 5);
}

TEST(YamlConfig, SampleFile) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path_to_file = dir.GetPath() + "/read_file_sample";

  /// [sample read_file]
  fs::blocking::RewriteFileContents(path_to_file, R"(some_key: ['a', 'b'])");
  const auto yaml_content = fmt::format("some#file: {}", path_to_file);
  auto node = formats::yaml::FromString(yaml_content);

  yaml_config::YamlConfig yaml(
      std::move(node), {}, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
  EXPECT_EQ(yaml["some"]["some_key"][0].As<std::string>(), "a");
  /// [sample read_file]
}

TEST(YamlConfig, FileFallback) {
  auto node = formats::yaml::FromString(R"(
some_element:
    some#file: /some/path/to/the/file.yaml
    some#fallback: 5
  )");

  yaml_config::YamlConfig yaml(
      std::move(node), {}, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 5);
}

TEST(YamlConfig, FileFallbackUnallowed) {
  auto node = formats::yaml::FromString(R"(
some_element:
    some#file: /some/path/to/the/file.yaml
    some#fallback: 5
  )");

  yaml_config::YamlConfig yaml(node, {},
                               yaml_config::YamlConfig::Mode::kEnvAllowed);
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);

  yaml = yaml_config::YamlConfig{node, {}, {}};
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);
}

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

TEST(YamlConfig, ExplicitStringType) {
  // Note: yaml-cpp 0.6.3 and older fails to parse `!!str null` as a string
  // https://github.com/jbeder/yaml-cpp/issues/590
  const yaml_config::YamlConfig conf{
      formats::yaml::FromString(R"(
        quoted-int: '123'
        str-int: !!str 123
        quoted-float: '123.5'
        str-float: !!str 123.5
        quoted-bool: 'true'
        str-bool: !!str true
        quoted-null: 'null'
        # str-null: !!str null
      )"),
      {},
  };

  const std::pair<std::string, std::string> kExpectedValues[]{
      {"quoted-int", "123"},     {"str-int", "123"},
      {"quoted-float", "123.5"}, {"str-float", "123.5"},
      {"quoted-bool", "true"},   {"str-bool", "true"},
      {"quoted-null", "null"},
  };

  for (const auto& [key_raw, expected_value_raw] : kExpectedValues) {
    using Exception = formats::yaml::TypeMismatchException;

    // UEXPECT_THROW is implemented using a lambda, which refuses to capture
    // structured bindings in C++17.
    const auto& key = key_raw;
    const auto& expected_value = expected_value_raw;

    EXPECT_TRUE(conf[key].IsString());
    UEXPECT_NO_THROW(EXPECT_EQ(conf[key].As<std::string>(), expected_value));

    EXPECT_FALSE(conf[key].IsInt());
    UEXPECT_THROW(conf[key].As<int>(), Exception);
    EXPECT_FALSE(conf[key].IsInt64());
    UEXPECT_THROW(conf[key].As<std::int64_t>(), Exception);
    EXPECT_FALSE(conf[key].IsUInt64());
    UEXPECT_THROW(conf[key].As<std::uint64_t>(), Exception);
    EXPECT_FALSE(conf[key].IsDouble());
    UEXPECT_THROW(conf[key].As<double>(), Exception);
    EXPECT_FALSE(conf[key].IsBool());
    UEXPECT_THROW(conf[key].As<bool>(), Exception);
    EXPECT_FALSE(conf[key].IsNull());
  }

  const auto json = conf.As<formats::json::Value>();
  EXPECT_EQ(json, formats::json::FromString(R"(
    {
      "quoted-int": "123",
      "str-int": "123",
      "quoted-float": "123.5",
      "str-float": "123.5",
      "quoted-bool": "true",
      "str-bool": "true",
      "quoted-null": "null"
    }
  )")) << "Actual json value: "
       << ToString(json);
}

TEST(YamlConfig, SampleMultipleWithVarsEnv) {
  const auto node = formats::yaml::FromString(R"(
# /// [sample multiple]
# yaml
some_element:
    some: $variable
    some#file: /some/path/to/the/file.yaml
    some#env: SOME_ENV_VARIABLE
    some#fallback: 100500
# /// [sample multiple]
  )");
  const auto vars = formats::yaml::FromString("variable#env: VARIABLE_ENV");

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::setenv("SOME_ENV_VARIABLE", "100", 1);

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::setenv("VARIABLE_ENV", "42", 1);

  yaml_config::YamlConfig yaml(node, vars);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);

  yaml = yaml_config::YamlConfig(node, vars,
                                 yaml_config::YamlConfig::Mode::kEnvAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);

  yaml = yaml_config::YamlConfig(
      node, vars, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 42);

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::unsetenv("VARIABLE_ENV");

  yaml = yaml_config::YamlConfig(node, {},
                                 yaml_config::YamlConfig::Mode::kEnvAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 100);

  yaml = yaml_config::YamlConfig(node, {});
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::unsetenv("SOME_ENV_VARIABLE");

  yaml = yaml_config::YamlConfig(node, {},
                                 yaml_config::YamlConfig::Mode::kEnvAllowed);
  UEXPECT_THROW(yaml["some_element"]["some"].As<int>(), std::exception);

  yaml = yaml_config::YamlConfig(
      node, {}, yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
  EXPECT_EQ(yaml["some_element"]["some"].As<int>(), 100500);
}

USERVER_NAMESPACE_END
