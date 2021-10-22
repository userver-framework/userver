#include <gtest/gtest.h>

#include <formats/common/roundtrip_test.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Roundtrip,
                               formats::json::ValueBuilder);

USERVER_NAMESPACE_END
