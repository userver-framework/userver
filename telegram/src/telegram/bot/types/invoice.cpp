#include <userver/telegram/bot/types/invoice.hpp>

#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
Invoice Parse(const Value& data, formats::parse::To<Invoice>) {
  return Invoice{
    data["title"].template As<std::string>(),
    data["description"].template As<std::string>(),
    data["start_parameter"].template As<std::string>(),
    data["currency"].template As<std::string>(),
    data["total_amount"].template As<std::int64_t>()
  };
}

template <class Value>
Value Serialize(const Invoice& invoice, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["title"] = invoice.title;
  builder["description"] = invoice.description;
  builder["start_parameter"] = invoice.start_parameter;
  builder["currency"] = invoice.currency;
  builder["total_amount"] = invoice.total_amount;
  return builder.ExtractValue();
}

}  // namespace impl

Invoice Parse(const formats::json::Value& json,
              formats::parse::To<Invoice> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const Invoice& invoice,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(invoice, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
