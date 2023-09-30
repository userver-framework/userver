#pragma once

#include <userver/telegram/bot/types/passport_file.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Describes documents or other Telegram Passport elements
/// shared with the bot by the user.
/// @see https://core.telegram.org/bots/api#encryptedpassportelement
struct EncryptedPassportElement {
  enum class Type {
    kPersonalDetails, kPassport, kDriverLicense, kIdentityCard,
    kInternalPassport, kAddress, kUtilityBill, kBankStatement, kRentalAgreement,
    kPassportRegistration, kTemporaryRegistration, kPhoneNumber, kEmail
  };

  /// @brief Element type.
  Type type;

  /// @brief Optional. Base64-encoded encrypted Telegram Passport element data
  /// provided by the user.
  /// @note Available for Type::kPersonalDetails, Type::kPassport,
  /// Type::kDriverLicense, Type::kIdentityCard, Type::kInternalPassport and 
  /// Type::kAddress.
  /// @note Can be decrypted and verified using the
  /// accompanying EncryptedCredentials.
  std::optional<std::string> data;

  /// @brief Optional. User's verified phone number.
  /// @note Available only for Type::kPhoneNumber.
  std::optional<std::string> phone_number;

  /// @brief Optional. User's verified email address.
  /// @note Available only for Type::kEmail
  std::optional<std::string> email;

  /// @brief Optional. Array of encrypted files with documents
  /// provided by the user.
  /// @note Available for Type::kUtilityBill, Type::kBankStatement,
  /// Type::kRentalAgreement, Type::kPassportRegistration and
  /// Type::kTemporaryRegistration.
  /// @note Files can be decrypted and verified using the
  /// accompanying EncryptedCredentials.
  std::optional<std::vector<PassportFile>> files;

  /// @brief Optional. Encrypted file with the front side of the document,
  /// provided by the user.
  /// @note Available for Type::kPassport, Type::kDriverLicense,
  /// Type::kIdentityCard and Type::kInternalPassport.
  /// @note The file can be decrypted and verified using the
  /// accompanying EncryptedCredentials.
  std::unique_ptr<PassportFile> front_side;

  /// @brief Optional. Encrypted file with the reverse side of the document,
  /// provided by the user.
  /// @note Available for Type::kDriverLicense and Type::kIdentityCard.
  /// @note The file can be decrypted and verified using the
  /// accompanying EncryptedCredentials.
  std::unique_ptr<PassportFile> reverse_side;

  /// @brief Optional. Encrypted file with the selfie of the user holding
  /// a document, provided by the user.
  /// @note Available for Type::kPassport, Type::kDriverLicense,
  /// Type::kIdentityCard and Type::kInternalPassport.
  /// @note The file can be decrypted and verified using the
  /// accompanying EncryptedCredentials.
  std::unique_ptr<PassportFile> selfie;

  /// @brief Optional. Array of encrypted files with translated versions of
  /// documents provided by the user.
  /// @note Available if requested for Type::kPassport, Type::kDriverLicense,
  /// Type::kIdentityCard, Type::kInternalPassport, Type::kUtilityBill,
  /// Type::kBankStatement, Type::kRentalAgreement, Type::kPassportRegistration
  /// and Type::kTemporaryRegistration.
  /// @note Files can be decrypted and verified using the
  /// accompanying EncryptedCredentials.
  std::optional<std::vector<PassportFile>> translation;

  /// @brief Base64-encoded element hash for using
  /// in PassportElementErrorUnspecified
  std::string hash;
};

std::string_view ToString(EncryptedPassportElement::Type type);

EncryptedPassportElement::Type Parse(const formats::json::Value& value,
                                     formats::parse::To<EncryptedPassportElement::Type>);

formats::json::Value Serialize(EncryptedPassportElement::Type type,
                               formats::serialize::To<formats::json::Value>);

EncryptedPassportElement Parse(const formats::json::Value& json,
                               formats::parse::To<EncryptedPassportElement>);

formats::json::Value Serialize(const EncryptedPassportElement& encrypted_passport_element,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
