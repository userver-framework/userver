#include <userver/utest/utest.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/formats/json.hpp>

#include <userver/baggage/baggage_manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class BaggageManagerTest : public ::testing::Test {
 protected:
  dynamic_config::StorageMock storage_{
      {baggage::kBaggageSettings, {{"key1", "key2", "key3", "key4", "key5"}}},
      {baggage::kBaggageEnabled, true}};
  dynamic_config::Source source_ = storage_.GetSource();
  baggage::BaggageManager baggage_manager_ = baggage::BaggageManager(source_);
};

void SetBaggageForTests(const baggage::BaggageManager& baggage_manager) {
  std::string header =
      "key1=value1,  key2 = value2  ; property1; PropertyKey2 "
      "=PropertyValue2; property3  ,key3 =  value3  ;\t property4 ;PropertyKey5"
      " =  PropertyValue5";
  baggage_manager.SetBaggage(std::move(header));
}

}  // namespace

// Test getter
UTEST_F(BaggageManagerTest, GetInheritedBaggage) {
  ASSERT_EQ(baggage::BaggageManager::TryGetBaggage(), nullptr);

  SetBaggageForTests(baggage_manager_);

  const auto& baggage = baggage::BaggageManager::TryGetBaggage();
  EXPECT_NE(baggage, nullptr);
  ASSERT_EQ(baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5");
}

// Test setter
UTEST_F(BaggageManagerTest, SetInheritedBaggage) {
  std::string new_header = "key1=value2;property1";
  baggage_manager_.SetBaggage(std::move(new_header));

  const auto& baggage = baggage::BaggageManager::TryGetBaggage();
  EXPECT_NE(baggage, nullptr);
  ASSERT_EQ(baggage->ToString(), "key1=value2;property1");
}

// Test AddEntry and successful allocation
UTEST_F(BaggageManagerTest, AddEntry) {
  SetBaggageForTests(baggage_manager_);

  baggage_manager_.AddEntry(
      "key4", "value4",
      {{"PropertyKey6", {"PropertyValue6"}}, {"property7", {}}});

  const auto& baggage = baggage::BaggageManager::TryGetBaggage();
  EXPECT_NE(baggage, nullptr);
  ASSERT_EQ(baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5,key4=value4;PropertyKey6=PropertyValue6;property7");

  baggage_manager_.AddEntry(
      "key5", "value5",
      {{"property8", {}}, {"PropertyKey9", {"PropertyValue9"}}});

  const auto& last_baggage = baggage::BaggageManager::TryGetBaggage();
  EXPECT_NE(last_baggage, nullptr);
  ASSERT_EQ(last_baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5,key4=value4;PropertyKey6=PropertyValue6;property7,key5=v"
            "alue5;property8;PropertyKey9=PropertyValue9");

  UEXPECT_THROW(baggage_manager_.AddEntry("key20", "value4", {}),
                baggage::BaggageException);
}

// Test Reset
UTEST_F(BaggageManagerTest, ResetBaggage) {
  SetBaggageForTests(baggage_manager_);

  baggage::BaggageManager::ResetBaggage();
  const auto& baggage = baggage::BaggageManager::TryGetBaggage();
  ASSERT_EQ(baggage, nullptr);
}

USERVER_NAMESPACE_END
