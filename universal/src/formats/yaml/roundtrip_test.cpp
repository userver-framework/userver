#include <gtest/gtest.h>

#include <formats/common/roundtrip_test.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, Roundtrip,
                               formats::yaml::ValueBuilder);

USERVER_NAMESPACE_END
