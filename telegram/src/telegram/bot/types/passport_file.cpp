#include <userver/telegram/bot/types/passport_file.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
PassportFile Parse(const Value& data, formats::parse::To<PassportFile>) {
  return PassportFile{
    data["file_id"].template As<std::string>(),
    data["file_unique_id"].template As<std::string>(),
    data["file_size"].template As<std::int64_t>(),
    data["file_date"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const PassportFile& passport_file,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["file_id"] = passport_file.file_id;
  builder["file_unique_id"] = passport_file.file_unique_id;
  builder["file_size"] = passport_file.file_size;
  builder["file_date"] = passport_file.file_date;
  return builder.ExtractValue();
}

}  // namespace impl

PassportFile Parse(const formats::json::Value& json,
                   formats::parse::To<PassportFile> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const PassportFile& passport_file,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(passport_file, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
