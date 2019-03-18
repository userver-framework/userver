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
  )"));

  auto node = formats::yaml::FromString(R"(
    string: $string
    duration1: $duration1
    duration2: $duration2
    duration3: $duration3
    int: $int
  )");

  yaml_config::YamlConfig conf(std::move(node), ".", std::move(vmap));
  EXPECT_EQ(conf.ParseString("string"), "hello");
  EXPECT_EQ(conf.ParseDuration("duration1"), std::chrono::seconds(10));
  EXPECT_EQ(conf.ParseDuration("duration2"), std::chrono::milliseconds(10));
  EXPECT_EQ(conf.ParseDuration("duration3"), std::chrono::seconds(1));
  EXPECT_EQ(conf.ParseInt("int"), 42);
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
