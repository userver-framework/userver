#include <userver/utest/utest.hpp>

#include <userver/baggage/baggage.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
// Check that functions GetValue, GetKey, GetProperties and GetEntries
// working stable and returning valid strings (converted from string_views)
std::string PrintBaggage(const baggage::Baggage& baggage) {
  std::string result = "Baggage:";
  for (const auto& entry : baggage.GetEntries()) {
    result += "\nEntry: " + entry.GetKey() + " " + entry.GetValue();
    for (const auto& property : entry.GetProperties()) {
      result += "\n\tProperty: " + property.GetKey();
      auto property_value = property.GetValue();
      if (property_value) {
        result += " " + *property_value;
      }
    }
  }
  return result;
}

inline const std::unordered_set<std::string> kAllowedKeys = {
    "key1", "key2", "key3", "key4", "key5", "key,5"};
}  // namespace

// Check header for valid string. Also check that after adding new entry,
// header allocation working correctly. Common test
UTEST(Baggage, ValidHeader) {
  std::string header =
      "key1=value1,  key2 = value2  ; property1; PropertyKey2 "
      "=PropertyValue2; property3  ,key3 =  value3  ;\t property4 ;PropertyKey5"
      " =  PropertyValue5";

  auto baggage = baggage::TryMakeBaggage(std::move(header), kAllowedKeys);
  ASSERT_EQ(baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5");
  ASSERT_EQ(PrintBaggage(*baggage),
            "Baggage:"
            "\nEntry: key1 value1"
            "\nEntry: key2 value2"
            "\n\tProperty: property1"
            "\n\tProperty: PropertyKey2 PropertyValue2"
            "\n\tProperty: property3"
            "\nEntry: key3 value3"
            "\n\tProperty: property4"
            "\n\tProperty: PropertyKey5 PropertyValue5");

  baggage->AddEntry("key4", "value4",
                    {{"PropertyKey6", {"PropertyValue6"}}, {"property7", {}}});
  ASSERT_EQ(baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5,key4=value4;PropertyKey6=PropertyValue6;property7");
  ASSERT_EQ(PrintBaggage(*baggage),
            "Baggage:"
            "\nEntry: key1 value1"
            "\nEntry: key2 value2"
            "\n\tProperty: property1"
            "\n\tProperty: PropertyKey2 PropertyValue2"
            "\n\tProperty: property3"
            "\nEntry: key3 value3"
            "\n\tProperty: property4"
            "\n\tProperty: PropertyKey5 PropertyValue5"
            "\nEntry: key4 value4"
            "\n\tProperty: PropertyKey6 PropertyValue6"
            "\n\tProperty: property7");

  // Test with decoded entry
  baggage->AddEntry(
      "key,5", "value,5",
      {{"property8", {}}, {"Property,Key9", {"Property,Value9"}}});
  ASSERT_EQ(baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5,key4=value4;PropertyKey6=PropertyValue6;property7,key"
            "%2C5=value%2C5;property8;Property%2CKey9=Property%2CValue9");
  ASSERT_EQ(PrintBaggage(*baggage),
            "Baggage:"
            "\nEntry: key1 value1"
            "\nEntry: key2 value2"
            "\n\tProperty: property1"
            "\n\tProperty: PropertyKey2 PropertyValue2"
            "\n\tProperty: property3"
            "\nEntry: key3 value3"
            "\n\tProperty: property4"
            "\n\tProperty: PropertyKey5 PropertyValue5"
            "\nEntry: key4 value4"
            "\n\tProperty: PropertyKey6 PropertyValue6"
            "\n\tProperty: property7"
            "\nEntry: key,5 value,5"
            "\n\tProperty: property8"
            "\n\tProperty: Property,Key9 Property,Value9");
}

// Check header for invalid entries. Also check invalid arguments when
// trying to add new entries. Common test
UTEST(Baggage, InvalidKeys) {
  std::string header =
      ",key1=,  key2 =   ; property1; PropertyKey2 "
      "=PropertyValue2; property3  ,, =  value3  ;\t property4 ;PropertyKey5"
      " =  ,key4=value=4,=,key6=value6,";

  auto baggage = baggage::TryMakeBaggage(std::move(header), kAllowedKeys);
  ASSERT_EQ(baggage->ToString(), "");
  ASSERT_EQ(baggage->GetEntries().size(), 0);

  UEXPECT_THROW(baggage->AddEntry("key4=rew", "value4",
                                  {{"Property;Key=6", {"Property;Value6"}},
                                   {"property,7", {}}}),
                baggage::BaggageException);
  ASSERT_EQ(baggage->ToString(), "");
}

// Check header with invalid properties. Common test. Check
// that key and value are not empty if property contains '='
UTEST(Baggage, InvalidProperties) {
  std::string header =
      "key1=value1  ;; ==;  ;=,  key2=value2  ;==;pk=,key3=value3;;=pv";

  auto baggage = baggage::TryMakeBaggage(std::move(header), kAllowedKeys);
  ASSERT_EQ(baggage->ToString(), "key1=value1,key2=value2,key3=value3");
  ASSERT_EQ(PrintBaggage(*baggage),
            "Baggage:"
            "\nEntry: key1 value1"
            "\nEntry: key2 value2"
            "\nEntry: key3 value3");
}

UTEST(Baggage, WithoutCommas) {
  std::string header = "key1=value1;property";

  auto baggage = baggage::TryMakeBaggage(std::move(header), kAllowedKeys);
  ASSERT_EQ(baggage->ToString(), "key1=value1;property");
  ASSERT_EQ(PrintBaggage(*baggage),
            "Baggage:"
            "\nEntry: key1 value1"
            "\n\tProperty: property");
}

// Check HasEntry, GetEntry, HasProperty and GetProperty
UTEST(Baggage, FindEntities) {
  std::string header =
      "key1=value1,  key%2C5 = value%2C5  ; property%2C1; PropertyKey%2C2 "
      "=PropertyValue%2C2; property3  ,key3 =  value3  ;\t property4 ;"
      "PropertyKey5 =  PropertyValue5";
  auto baggage = baggage::TryMakeBaggage(std::move(header), kAllowedKeys);

  ASSERT_EQ(baggage->HasEntry("key,5"), true);
  auto entry = baggage->GetEntry("key,5");
  ASSERT_EQ(entry.GetValue(), "value,5");

  ASSERT_EQ(entry.HasProperty("PropertyKey,2"), true);
  auto property_with_value = entry.GetProperty("PropertyKey,2");
  ASSERT_EQ(property_with_value.GetValue(), "PropertyValue,2");

  auto property_without_value = entry.GetProperty("property,1");
  ASSERT_EQ(property_without_value.GetValue(), std::nullopt);

  ASSERT_EQ(entry.HasProperty("property4"), false);
  UEXPECT_THROW(entry.GetProperty("property4"), baggage::BaggageException);

  ASSERT_EQ(baggage->HasEntry("key4"), false);
  UEXPECT_THROW(baggage->GetEntry("key4"), baggage::BaggageException);
}

// Check functions of available entries
UTEST(Baggage, AvailableEntries) {
  auto baggage = baggage::TryMakeBaggage("", kAllowedKeys);

  // check IsValidEntry
  ASSERT_EQ(baggage->IsValidEntry("key2"), true);
  ASSERT_EQ(baggage->IsValidEntry("key6"), false);

  // get available entries
  for (const auto& valid_entry : baggage->GetAvailableEntries()) {
    ASSERT_EQ(kAllowedKeys.count(valid_entry), 1);
  }
}

// Check header Parser
UTEST(Parse, BaggageHeader) {
  std::string header =
      "key1=value1,  key2 = value2  ; property1; PropertyKey2 "
      "=PropertyValue2; property3  ,key3 =  value3  ;\t property4 ;PropertyKey5"
      " =  PropertyValue5";
  auto baggage = baggage::TryMakeBaggage(std::move(header), kAllowedKeys);

  ASSERT_EQ(baggage->ToString(),
            "key1=value1,key2=value2;property1;PropertyKey2"
            "=PropertyValue2;property3,key3=value3;property4;PropertyKey5=Prope"
            "rtyValue5");
  ASSERT_EQ(PrintBaggage(*baggage),
            "Baggage:"
            "\nEntry: key1 value1"
            "\nEntry: key2 value2"
            "\n\tProperty: property1"
            "\n\tProperty: PropertyKey2 PropertyValue2"
            "\n\tProperty: property3"
            "\nEntry: key3 value3"
            "\n\tProperty: property4"
            "\n\tProperty: PropertyKey5 PropertyValue5");

  // exceed limit
  std::string invalid_header(9000, ',');
  ASSERT_EQ(std::nullopt,
            baggage::TryMakeBaggage(std::move(invalid_header), kAllowedKeys));

  // empty header
  auto empty_baggage = baggage::TryMakeBaggage("", kAllowedKeys);
  ASSERT_EQ(empty_baggage->ToString(), "");

  // header contains only spaces
  std::string header_with_spaces = "  \t ";
  auto baggage_with_spaces =
      baggage::TryMakeBaggage(std::move(header_with_spaces), kAllowedKeys);
  ASSERT_EQ(baggage_with_spaces->ToString(), "");
}

class UTestBaggage : public baggage::Baggage {
 public:
  UTestBaggage(const std::unordered_set<std::string>& allowed_keys)
      : baggage::Baggage("", allowed_keys) {}
  void UTestTryMakeBaggageEntry() {
    // valid entry with properties
    std::string entry_str = "key3=value3;property4;PropertyKey5=PropertyValue5";
    auto entry = this->TryMakeBaggageEntry(entry_str);
    ASSERT_EQ(entry->ToString(),
              "key3=value3;property4;PropertyKey5=PropertyValue5");
    ASSERT_EQ(entry->GetKey(), "key3");
    ASSERT_EQ(entry->GetValue(), "value3");
    ASSERT_EQ(entry->GetProperty("property4").GetKey(), "property4");
    ASSERT_EQ(entry->GetProperty("property4").GetValue(), std::nullopt);
    ASSERT_EQ(entry->GetProperty("PropertyKey5").GetKey(), "PropertyKey5");
    ASSERT_EQ(entry->GetProperty("PropertyKey5").GetValue(), "PropertyValue5");

    // valid entry without properties
    entry_str = "key3=value3";
    auto entry_without_properties = this->TryMakeBaggageEntry(entry_str);
    ASSERT_EQ(entry_without_properties->ToString(), "key3=value3");
    ASSERT_EQ(entry_without_properties->GetProperties().size(), 0);

    // valid entry, but invalid properties
    entry_str = "key1=value1;;=propertyValue2;propertyKey3=";
    auto entry_with_invalid_properties = this->TryMakeBaggageEntry(entry_str);
    ASSERT_EQ(entry_with_invalid_properties->ToString(), "key1=value1");
    ASSERT_EQ(entry_with_invalid_properties->GetProperties().size(), 0);

    // missing entry's value. Contains properties
    std::string invalid_entry_str =
        "key3=;property4;PropertyKey5=PropertyValue5";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // missing entry's value. Doesn't contain properties
    invalid_entry_str = "key3=";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // missing entry's key. Contains properties
    invalid_entry_str = "=value3;property4;PropertyKey5=PropertyValue5";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // missing entry's key. Doesn't contain properties
    invalid_entry_str = "=value3";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // check that entry doesn't contain ','
    invalid_entry_str = "key3=value3;property4;Prope,rtyKey5=PropertyValue5";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // check entry key, which doesn't contain in availbale_entries
    invalid_entry_str = "key6=value3;property4;PropertyKey5=PropertyValue5";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // missing '='
    invalid_entry_str = "key6";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // entry contains spaces
    invalid_entry_str = "\t";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));

    // empty entry
    invalid_entry_str = "";
    ASSERT_EQ(std::nullopt, this->TryMakeBaggageEntry(invalid_entry_str));
  }

  static void UTestTryMakeBaggageProperty() {
    // check parsing property without value
    std::string property_str = "property";
    auto property = UTestBaggage::TryMakeBaggageEntryProperty(property_str);
    ASSERT_EQ(property->ToString(), "property");
    ASSERT_EQ(property->GetKey(), "property");
    ASSERT_EQ(property->GetValue(), std::nullopt);

    // check parsing property with value
    std::string property_with_value_str = "propertyKey=propertyValue";
    auto property_with_value =
        UTestBaggage::TryMakeBaggageEntryProperty(property_with_value_str);
    ASSERT_EQ(property_with_value->ToString(), "propertyKey=propertyValue");
    ASSERT_EQ(property_with_value->GetKey(), "propertyKey");
    ASSERT_EQ(*property_with_value->GetValue(), "propertyValue");

    // property contains ;
    std::string invalid_property_str = "property4;PropertyKey5=PropertyValue5";
    ASSERT_EQ(std::nullopt,
              UTestBaggage::TryMakeBaggageEntryProperty(invalid_property_str));

    // property contains ,
    invalid_property_str = ",property4PropertyKey5=PropertyValue5";
    ASSERT_EQ(std::nullopt,
              UTestBaggage::TryMakeBaggageEntryProperty(invalid_property_str));

    // empty key
    invalid_property_str = "=PropertyValue5";
    ASSERT_EQ(std::nullopt,
              UTestBaggage::TryMakeBaggageEntryProperty(invalid_property_str));

    // empty value
    invalid_property_str = "PropertyKey5=";
    ASSERT_EQ(std::nullopt,
              UTestBaggage::TryMakeBaggageEntryProperty(invalid_property_str));

    // property contains spaces
    invalid_property_str = "\t";
    ASSERT_EQ(std::nullopt,
              UTestBaggage::TryMakeBaggageEntryProperty(invalid_property_str));

    // empty property
    invalid_property_str = "";
    ASSERT_EQ(std::nullopt,
              UTestBaggage::TryMakeBaggageEntryProperty(invalid_property_str));
  }
};

UTEST(Baggage, Parsers) {
  UTestBaggage tester(kAllowedKeys);
  tester.UTestTryMakeBaggageEntry();
  UTestBaggage::UTestTryMakeBaggageProperty();
}

USERVER_NAMESPACE_END
