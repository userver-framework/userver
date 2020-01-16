#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <formats/json/types.hpp>
#include <formats/json/value_builder.hpp>

// These tests ensure that array/object members are internally stored in plain
// contiguous array. This assumption used in `json::Value::GetPath` to find
// element pointer in O(1) instead of naive O(n) search.

namespace {
formats::json::impl::DefaultAllocator g_crt_allocator;
}

// Ensure contiguous allocation in rapidjson arrays
TEST(FormatsJson, RapidjsonContiguousArrays) {
  using formats::json::impl::Value;
  Value json{rapidjson::kArrayType};

  for (int i = 0; i < 1000; i++) json.PushBack(i, g_crt_allocator);

  Value* begin = &*json.Begin();
  for (int i = 0, size = json.Size(); i < size; i++) {
    ASSERT_EQ(&json[i], begin + i);
  }
}

// Ensure contiguous allocation in rapidjson objects
TEST(FormatsJson, RapidjsonContiguousMaps) {
  using formats::json::impl::Value;
  Value json{rapidjson::kObjectType};

  for (int i = 0; i < 1000; i++)
    json.AddMember(Value{std::to_string(i), g_crt_allocator}, Value{i},
                   g_crt_allocator);

  auto begin = &*json.MemberBegin();
  for (int i = 0, size = json.MemberCount(); i < size; i++) {
    std::string index = std::to_string(i);
    ASSERT_EQ(&json[index], &begin[i].value);
  }
}
