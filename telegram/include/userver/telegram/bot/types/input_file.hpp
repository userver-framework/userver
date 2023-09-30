#pragma once

#include <memory>
#include <string_view>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Form;
}  // namespace clients::http

namespace telegram::bot {

/// @brief This object represents the contents of a file to be uploaded.
/// @see https://core.telegram.org/bots/api#inputfile
struct InputFile {
  InputFile(std::string _data,
            std::string _file_name,
            std::string _content_type);

  InputFile(std::shared_ptr<std::string> _data,
            std::string _file_name,
            std::string _content_type);

  /// @brief File content.
  /// @note 10 MB max size for photos, 50 MB for other files.
  std::shared_ptr<std::string> data;

  /// @brief File name.
  std::string file_name;

  /// @brief File content type.
  std::string content_type;
};

void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const InputFile& field);

InputFile Parse(const formats::json::Value& json,
                formats::parse::To<InputFile> to);

formats::json::Value Serialize(const InputFile& input_file,
                               formats::serialize::To<formats::json::Value> to);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
