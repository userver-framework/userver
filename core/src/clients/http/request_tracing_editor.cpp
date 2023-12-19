#include <userver/clients/http/request_tracing_editor.hpp>

#include <curl-ev/easy.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

RequestTracingEditor::RequestTracingEditor(curl::easy& easy) : easy_(easy) {}

void RequestTracingEditor::SetHeader(std::string_view name,
                                     std::string_view value) {
  easy_.add_header(name, value, curl::easy::EmptyHeaderAction::kDoNotSend,
                   curl::easy::DuplicateHeaderAction::kReplace);
}

}  // namespace clients::http

USERVER_NAMESPACE_END
