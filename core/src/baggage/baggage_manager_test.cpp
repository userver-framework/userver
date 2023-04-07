#include <userver/utest/utest.hpp>

#include <userver/baggage/baggage_manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

inline std::string kBaggageCfg = R"~({
  "BAGGAGE_SETTINGS": {
    "allowed_keys": [
      "key1",
      "key2",
      "key3",
      "key4",
      "key5"
    ]
  }
})~";

baggage::BaggageManager CreateBaggageManager() {
  dynamic_config::DocsMap docs_map;
  docs_map.Parse(kBaggageCfg, false);
  auto settings = baggage::BaggageSettings::Parse(docs_map);

  return baggage::BaggageManager(settings);
}

void CreateInheritedVariable() {
  std::string header =
      "key1=value1,  key2 = value2  ; property1; PropertyKey2 "
      "=PropertyValue2; property3  ,key3 =  value3  ;\t property4 ;PropertyKey5"
      " =  PropertyValue5";

  auto baggage = baggage::TryMakeBaggage(
      std::move(header), {"key1", "key2", "key3", "key4", "key5"});
  baggage::kInheritedBaggage.Set(std::move(*baggage));
}
}  // namespace

// Test getter
UTEST(BaggageManager, GetInheritedBaggage) {
  UEXPECT_THROW(baggage::BaggageManager::GetBaggage(), std::runtime_error);
  auto baggage_manager = CreateBaggageManager();
  CreateInheritedVariable();

  const auto& baggage = baggage::BaggageManager::GetBaggage();
  ASSERT_EQ(baggage.ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5");
}

// Test setter
UTEST(BaggageManager, SetInheritedBaggage) {
  auto baggage_manager = CreateBaggageManager();
  std::string new_header = "key1=value2;property1";
  baggage_manager.SetBaggage(std::move(new_header));

  const auto& baggage = baggage::BaggageManager::GetBaggage();
  ASSERT_EQ(baggage.ToString(), "key1=value2;property1");
}

// Test AddEntry and successful allocation
UTEST(BaggageManager, AddEntry) {
  CreateInheritedVariable();
  auto baggage_manager = CreateBaggageManager();

  baggage_manager.AddEntry(
      "key4", "value4",
      {{"PropertyKey6", {"PropertyValue6"}}, {"property7", {}}});

  const auto& baggage = baggage::BaggageManager::GetBaggage();
  ASSERT_EQ(baggage.ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5,key4=value4;PropertyKey6=PropertyValue6;property7");

  baggage_manager.AddEntry(
      "key5", "value5",
      {{"property8", {}}, {"PropertyKey9", {"PropertyValue9"}}});

  const auto& last_baggage = baggage::BaggageManager::GetBaggage();
  ASSERT_EQ(last_baggage.ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5,key4=value4;PropertyKey6=PropertyValue6;property7,key5=v"
            "alue5;property8;PropertyKey9=PropertyValue9");

  UEXPECT_THROW(baggage_manager.AddEntry("key20", "value4", {}),
                baggage::BaggageException);
}

// Test Reset
UTEST(BaggageManager, ResetBaggage) {
  CreateInheritedVariable();
  auto baggage_manager = CreateBaggageManager();

  baggage_manager.ResetBaggage();

  const auto& baggage = baggage::BaggageManager::GetBaggage();
  ASSERT_EQ(baggage.ToString(), "");
}

USERVER_NAMESPACE_END
