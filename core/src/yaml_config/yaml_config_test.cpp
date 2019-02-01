#include <yaml_config/yaml_config.hpp>

#include <gtest/gtest.h>

TEST(YamlConfig, Basic) {
  yaml_config::VariableMapPtr empty_vmap{};

  auto node = YAML::Load(R"(
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
  auto vmap = std::make_shared<yaml_config::VariableMap>(YAML::Load(R"(
    string: hello
    duration1: 10s
    duration2: 10ms
    duration3: 1
    int: 42
  )"));

  auto node = YAML::Load(R"(
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
