#include <telegram/bot/requests/request_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const std::string& field) {
  LOG_INFO() << "field_name: " << field_name << "as string";
  form.AddContent(field_name,
                  field,
                  http::content_type::kTextPlain.ToString());
}

void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     bool field) {
  LOG_INFO() << "field_name: " << field_name << "as bool";
  form.AddContent(field_name,
                  fmt::format("{}", field),
                  http::content_type::kTextPlain.ToString());
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
