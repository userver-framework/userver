#include <gtest/gtest.h>

#include <formats/common/roundtrip_test.hpp>
#include <formats/json/value_builder.hpp>

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Roundtrip,
                               formats::json::ValueBuilder);
