#include <userver/clients/http/form.hpp>

#include <curl-ev/form.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

Form::Form() : impl_(std::make_shared<curl::form>()) {}
Form::~Form() = default;

void Form::AddContent(std::string_view key, std::string_view content) {
  impl_->add_content(key, content);
}

void Form::AddContent(std::string_view key, std::string_view content,
                      const std::string& content_type) {
  impl_->add_content(key, content, content_type);
}

void Form::AddBuffer(const std::string& key, const std::string& file_name,
                     const std::shared_ptr<std::string>& buffer) {
  impl_->add_buffer(key, file_name, buffer);
}

void Form::AddBuffer(const std::string& key, const std::string& file_name,
                     const std::shared_ptr<std::string>& buffer,
                     const std::string& content_type) {
  impl_->add_buffer(key, file_name, buffer, content_type);
}

const std::shared_ptr<curl::form>& Form::GetNative() const { return impl_; }

}  // namespace clients::http

USERVER_NAMESPACE_END
