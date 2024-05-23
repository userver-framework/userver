#include <userver/telegram/bot/types/input_file.hpp>

#include <userver/clients/http/form.hpp>
#include <userver/formats/json.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class T, class Value>
std::shared_ptr<T> Parse(const Value& value,
                         formats::parse::To<std::shared_ptr<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return nullptr;
  }
  return std::make_shared<T>(value.template As<T>());
}

template <class Value>
InputFile Parse(const Value& data, formats::parse::To<InputFile>) {
  return InputFile{
    Parse(data["data"], formats::parse::To<std::shared_ptr<std::string>>()),
    data["file_name"].template As<std::string>(),
    data["content_type"].template As<std::string>()
  };
}

template <class T, class Value>
Value Serialize(const std::shared_ptr<T>& value,
                formats::serialize::To<Value>) {
  if (!value) return {};

  return typename Value::Builder(*value).ExtractValue();
}

template <class Value>
Value Serialize(const InputFile& input_file, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["data"] = Serialize(input_file.data, formats::serialize::To<Value>());
  builder["file_name"] = input_file.file_name;
  builder["content_type"] = input_file.content_type;
  return builder.ExtractValue();
}

}  // namespace impl

InputFile::InputFile(std::string _data,
                     std::string _file_name,
                     std::string _content_type)
    : data(std::make_shared<std::string>(std::move(_data)))
    , file_name(std::move(_file_name))
    , content_type(std::move(_content_type)) {}

InputFile::InputFile(std::shared_ptr<std::string> _data,
                     std::string _file_name,
                     std::string _content_type)
    : data(_data)
    , file_name(std::move(_file_name))
    , content_type(std::move(_content_type)) {}

void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const InputFile& field) {
  LOG_INFO() << "field_name: " << field_name << "as input file";
  form.AddBuffer(field_name, field.file_name, field.data, field.content_type);
}

InputFile Parse(const formats::json::Value& json,
                formats::parse::To<InputFile> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(
    const InputFile& input_file,
    formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(input_file, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
