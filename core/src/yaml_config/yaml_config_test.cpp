#include <yaml_config/yaml_config.hpp>

#include <gtest/gtest.h>
#include <formats/yaml/serialize.hpp>

TEST(YamlConfig, Basic) {
  yaml_config::VariableMapPtr empty_vmap{};

  auto node = formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
  )");

  yaml_config::YamlConfig conf(std::move(node), ".", empty_vmap);
  EXPECT_EQ(conf.ParseString("string"), "hello");
  EXPECT_EQ(conf.ParseDuration("duration1"), std::chrono::seconds(10));
  EXPECT_EQ(conf.ParseDuration("duration2"), std::chrono::milliseconds(10));
  EXPECT_EQ(conf.ParseDuration("duration3"), std::chrono::seconds(1));
  EXPECT_EQ(conf.ParseInt("int"), 42);
}

TEST(YamlConfig, VariableMap) {
  auto vmap =
      std::make_shared<yaml_config::VariableMap>(formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
    elem0: str123
    not_missing: xxx
  )"));

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

  yaml_config::YamlConfig conf(std::move(node), ".", std::move(vmap));
  EXPECT_EQ(conf.ParseString("string"), "hello");
  EXPECT_EQ(conf.ParseDuration("duration1"), std::chrono::seconds(10));
  EXPECT_EQ(conf.ParseDuration("duration2"), std::chrono::milliseconds(10));
  EXPECT_EQ(conf.ParseDuration("duration3"), std::chrono::seconds(1));
  EXPECT_EQ(conf.ParseInt("int"), 42);
  std::vector<std::string> expected_vec{"str123", "str222"};
  EXPECT_EQ(conf.Parse<std::vector<std::string>>("array"), expected_vec);
  EXPECT_THROW(conf.Parse<std::vector<std::string>>("bad_array"),
               std::runtime_error);

  std::vector<std::optional<std::string>> expected_vec_miss{
      std::nullopt, std::optional<std::string>("str333"),
      std::optional<std::string>("xxx"), std::nullopt};
  EXPECT_EQ(
      conf.Parse<std::vector<std::optional<std::string>>>("array_with_missing"),
      expected_vec_miss);
}

TEST(YamlConfig, SquareBracketOperator) {
  auto vmap =
      std::make_shared<yaml_config::VariableMap>(formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
    test2:
      zzz: asd
    test4: qwe
  )"));

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
  yaml_config::YamlConfig conf(std::move(node), ".", std::move(vmap));

  auto root_array = conf["root"];
  EXPECT_TRUE(root_array.Yaml().IsArray());

  EXPECT_EQ(root_array[0].ParseInt("member1"), 42);
  EXPECT_EQ(root_array[0].ParseDuration("duration"), std::chrono::seconds{10});

  EXPECT_EQ(root_array[1].ParseString("member2"), "hello");
  EXPECT_EQ(root_array[1].ParseDuration("duration"),
            std::chrono::milliseconds{10});
  EXPECT_TRUE(root_array[1]["miss_val"].IsMissing());
  EXPECT_TRUE(root_array[1]["miss_val2"].IsMissing());
  EXPECT_THROW(root_array[1]["miss_val"].Yaml().As<std::string>(),
               formats::yaml::MemberMissingException);

  EXPECT_TRUE(root_array[1]["miss_val"]["sub_val"].IsMissing());
  EXPECT_TRUE(root_array[1]["miss_val2"]["sub_val"].IsMissing());
  EXPECT_EQ(
      root_array[1]["miss_val"].Parse<std::optional<std::string>>("sub_val"),
      std::optional<std::string>{});
  EXPECT_EQ(
      root_array[1]["miss_val2"].Parse<std::optional<std::string>>("sub_val"),
      std::optional<std::string>{});
  EXPECT_EQ(root_array[1]["miss_val"].Parse<std::optional<std::string>>(
                "sub_val", "def"),
            "def");
  EXPECT_EQ(root_array[1]["miss_val2"].Parse<std::optional<std::string>>(
                "sub_val", "def"),
            "def");
  EXPECT_TRUE(root_array[1]["miss_val"][11].IsMissing());
  EXPECT_TRUE(root_array[1]["miss_val2"][11].IsMissing());
  EXPECT_EQ(root_array[1]["miss_val"][11].Parse<std::optional<std::string>>(
                "sub_val", "def"),
            "def");
  EXPECT_EQ(root_array[1]["miss_val2"][11].Parse<std::optional<std::string>>(
                "sub_val", "def"),
            "def");
  EXPECT_THROW(root_array[1]["miss_val"][11].Parse<std::string>("sub_val"),
               std::runtime_error);
  EXPECT_THROW(root_array[1]["miss_val2"][11].Parse<std::string>("sub_val"),
               std::runtime_error);

  EXPECT_EQ(root_array[2].ParseString("zzz"), "asd");

  EXPECT_TRUE(root_array[3].Yaml().IsMissing());
  EXPECT_THROW(root_array[3].Yaml().As<std::string>(),
               formats::yaml::MemberMissingException);
  EXPECT_EQ(root_array[3].Yaml().As<std::string>("dflt"), "dflt");

  EXPECT_EQ(root_array[4].Yaml().As<std::string>(), "qwe");
  EXPECT_EQ(root_array[5].Yaml().As<std::string>(), "str5");

  EXPECT_THROW(root_array[99].ParseString("zzz"),
               formats::yaml::OutOfBoundsException);
}

TEST(YamlConfig, PathsParseInto) {
  auto vmap =
      std::make_shared<yaml_config::VariableMap>(formats::yaml::FromString(R"(
    path_vm: /hello/1
    path_array_vm: ["/hello/1", "/path/2"]
  )"));

  formats::yaml::Value node = formats::yaml::FromString(R"(
    path: /hello/1
    path_array: ["/hello/1", "/path/2"]
    path_vm: $path_vm
    path_array_vm: $path_array_vm
  )");

  yaml_config::YamlConfig conf(std::move(node), ".", std::move(vmap));

  auto path = conf.ParseString("path");
  EXPECT_EQ(path, "/hello/1");

  auto path_vm = conf.ParseString("path_vm");
  EXPECT_EQ(path_vm, "/hello/1");

  auto path_array = conf.Parse<std::vector<std::string>>("path_array");
  ASSERT_EQ(path_array.size(), 2);
  EXPECT_EQ(path_array[0], "/hello/1");
  EXPECT_EQ(path_array[1], "/path/2");

  auto path_array_vm = conf.Parse<std::vector<std::string>>("path_array_vm");
  ASSERT_EQ(path_array_vm.size(), 2);
  EXPECT_EQ(path_array_vm[0], "/hello/1");
  EXPECT_EQ(path_array_vm[1], "/path/2");
}

TEST(YamlConfig, Subconfig) {
  auto vmap =
      std::make_shared<yaml_config::VariableMap>(formats::yaml::FromString(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
  )"));

  auto node = formats::yaml::FromString(R"(
    member: $string
    subconfig:
      duration1: $duration1
      duration2: $duration2
  )");

  yaml_config::YamlConfig conf(std::move(node), ".", std::move(vmap));
  EXPECT_EQ(conf.ParseString("member"), "hello");
  // Get subconfig
  auto subconf = conf["subconfig"];
  EXPECT_EQ(subconf.ParseDuration("duration1"), std::chrono::seconds(10));
  EXPECT_EQ(subconf.ParseDuration("duration2"), std::chrono::milliseconds(10));

  // Check missing
  auto missing_subconf = conf["missing"];
  EXPECT_TRUE(missing_subconf.IsMissing());
}

TEST(YamlConfig, SubconfigNotObject) {
  auto vmap = std::make_shared<yaml_config::VariableMap>(
      formats::yaml::ValueBuilder(formats::common::Type::kObject)
          .ExtractValue());

  auto node = formats::yaml::FromString(R"(
    member: hello
  )");

  yaml_config::YamlConfig conf(std::move(node), ".", std::move(vmap));
  EXPECT_EQ(conf.ParseString("member"), "hello");
  EXPECT_TRUE(conf.Yaml().IsObject());
  // Get subconfig that is not an object
  auto subconf = conf["member"];
  // It must throw an exception
  EXPECT_ANY_THROW(subconf["another"]);
}
