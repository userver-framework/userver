#include <userver/utest/assert_macros.hpp>

// Goes before yaml_config.hpp to check that it compiles OK w/o yaml headers
#include <schemas/custom_cpp_type.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

TEST(YamlConfig, Presence) {
  static_assert(formats::common::impl::kHasParse<
                USERVER_NAMESPACE::yaml_config::YamlConfig, ns::CustomStruct1>);

  // Due to JSON extra
  static_assert(
      !formats::common::impl::kHasParse<
          USERVER_NAMESPACE::yaml_config::YamlConfig, ns::CustomAllOf__P0>);

  // Transitively
  static_assert(!formats::common::impl::kHasParse<
                USERVER_NAMESPACE::yaml_config::YamlConfig, ns::CustomAllOf>);
}

TEST(YamlConfig, Parse) {
  auto value = formats::yaml::FromString(R"--(
type: some string
field1: 123
)--");
  yaml_config::YamlConfig config{value, {}};
  EXPECT_EQ(config.As<ns::CustomStruct1>(),
            (ns::CustomStruct1{"some string", 123}));
}

USERVER_NAMESPACE_END
