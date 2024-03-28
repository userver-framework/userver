#include <userver/telegram/bot/types/encrypted_passport_element.hpp>

#include <telegram/bot/formats/parse.hpp>
#include <telegram/bot/formats/serialize.hpp>
#include <telegram/bot/formats/value_builder.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace {

constexpr utils::TrivialBiMap kEncryptedPassportElementTypeMap([](auto selector) {
  return selector()
    .Case(EncryptedPassportElement::Type::kPersonalDetails, "personal_details")
    .Case(EncryptedPassportElement::Type::kPassport, "passport")
    .Case(EncryptedPassportElement::Type::kDriverLicense, "driver_license")
    .Case(EncryptedPassportElement::Type::kIdentityCard, "identity_card")
    .Case(EncryptedPassportElement::Type::kInternalPassport, "internal_passport")
    .Case(EncryptedPassportElement::Type::kAddress, "address")
    .Case(EncryptedPassportElement::Type::kUtilityBill, "utility_bill")
    .Case(EncryptedPassportElement::Type::kBankStatement, "bank_statement")
    .Case(EncryptedPassportElement::Type::kRentalAgreement, "rental_agreement")
    .Case(EncryptedPassportElement::Type::kPassportRegistration, "passport_registration")
    .Case(EncryptedPassportElement::Type::kTemporaryRegistration, "temporary_registration")
    .Case(EncryptedPassportElement::Type::kPhoneNumber, "phone_number")
    .Case(EncryptedPassportElement::Type::kEmail, "email");
});

}  // namespace

namespace impl {

template <class Value>
EncryptedPassportElement Parse(const Value& data,
                               formats::parse::To<EncryptedPassportElement>) {
  return EncryptedPassportElement{
    data["type"].template As<EncryptedPassportElement::Type>(),
    data["data"].template As<std::optional<std::string>>(),
    data["phone_number"].template As<std::optional<std::string>>(),
    data["email"].template As<std::optional<std::string>>(),
    data["files"].template As<std::optional<std::vector<PassportFile>>>(),
    data["front_side"].template As<std::unique_ptr<PassportFile>>(),
    data["reverse_side"].template As<std::unique_ptr<PassportFile>>(),
    data["selfie"].template As<std::unique_ptr<PassportFile>>(),
    data["translation"].template As<std::optional<std::vector<PassportFile>>>(),
    data["hash"].template As<std::string>()
  };
}

template <class Value>
Value Serialize(const EncryptedPassportElement& encrypted_passport_element,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["type"] = encrypted_passport_element.type;
  SetIfNotNull(builder, "data", encrypted_passport_element.data);
  SetIfNotNull(builder, "phone_number", encrypted_passport_element.phone_number);
  SetIfNotNull(builder, "email", encrypted_passport_element.email);
  SetIfNotNull(builder, "files", encrypted_passport_element.files);
  SetIfNotNull(builder, "front_side", encrypted_passport_element.front_side);
  SetIfNotNull(builder, "reverse_side", encrypted_passport_element.reverse_side);
  SetIfNotNull(builder, "selfie", encrypted_passport_element.selfie);
  SetIfNotNull(builder, "translation", encrypted_passport_element.translation);
  builder["hash"] = encrypted_passport_element.hash;
  return builder.ExtractValue();
}

}  // namespace impl

std::string_view ToString(EncryptedPassportElement::Type type) {
  return utils::impl::EnumToStringView(type, kEncryptedPassportElementTypeMap);
}

EncryptedPassportElement::Type Parse(const formats::json::Value& value,
                                     formats::parse::To<EncryptedPassportElement::Type>) {
  return utils::ParseFromValueString(value, kEncryptedPassportElementTypeMap);
}

formats::json::Value Serialize(EncryptedPassportElement::Type type,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::ValueBuilder(ToString(type)).ExtractValue();
}

EncryptedPassportElement Parse(const formats::json::Value& json,
                               formats::parse::To<EncryptedPassportElement> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const EncryptedPassportElement& encrypted_passport_element,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(encrypted_passport_element, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
