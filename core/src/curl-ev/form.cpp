/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for constructing libcurl forms
*/

#include <curl-ev/error_code.hpp>
#include <curl-ev/form.hpp>
#include <curl-ev/native.hpp>
#include <curl-ev/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

form::form() { impl::CurlGlobal::Init(); }

form::~form() {
  if (post_) {
    native::curl_formfree(post_);
    post_ = nullptr;
  }
}

void form::add_content(const std::string& key, const std::string& content) {
  std::error_code ec;
  add_content(key, content, ec);
  throw_error(ec, "add_content");
}

void form::add_content(const std::string& key, const std::string& content,
                       std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_COPYCONTENTS,
      content.c_str(), native::CURLFORM_CONTENTSLENGTH, content.length(),
      native::CURLFORM_END))};
}

void form::add_content(const std::string& key, const std::string& content,
                       const std::string& content_type) {
  std::error_code ec;
  add_content(key, content, content_type, ec);
  throw_error(ec, "add_content");
}

void form::add_content(const std::string& key, const std::string& content,
                       const std::string& content_type, std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_COPYCONTENTS,
      content.c_str(), native::CURLFORM_CONTENTSLENGTH, content.length(),
      native::CURLFORM_CONTENTTYPE, content_type.c_str(),
      native::CURLFORM_END))};
}

void form::add_buffer(const std::string& key, const std::string& file_name,
                      const char* buffer, size_t buffer_len,
                      std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_BUFFER,
      file_name.c_str(), native::CURLFORM_BUFFERPTR, buffer,
      native::CURLFORM_BUFFERLENGTH, buffer_len, native::CURLFORM_END))};
}

void form::add_buffer(const std::string& key, const std::string& file_name,
                      const std::shared_ptr<std::string>& buffer) {
  std::error_code ec;
  add_buffer(key, file_name, buffer, ec);
  throw_error(ec, "add_buffer");
}

void form::add_buffer(const std::string& key, const std::string& file_name,
                      const std::shared_ptr<std::string>& buffer,
                      std::error_code& ec) {
  buffers_.push_back(buffer);
  add_buffer(key, file_name, buffers_.back()->c_str(), buffers_.back()->size(),
             ec);
}

void form::add_buffer(const std::string& key, const std::string& file_name,
                      const std::shared_ptr<std::string>& buffer,
                      const std::string& content_type) {
  std::error_code ec;
  add_buffer(key, file_name, buffer, content_type, ec);
  throw_error(ec, "add_buffer");
}

void form::add_buffer(const std::string& key, const std::string& file_name,
                      const std::shared_ptr<std::string>& buffer,
                      const std::string& content_type, std::error_code& ec) {
  buffers_.push_back(buffer);
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_BUFFER,
      file_name.c_str(), native::CURLFORM_BUFFERPTR, buffer->c_str(),
      native::CURLFORM_BUFFERLENGTH, buffer->length(),
      native::CURLFORM_CONTENTTYPE, content_type.c_str(),
      native::CURLFORM_END))};
}

void form::add_file(const std::string& key, const std::string& file_path) {
  std::error_code ec;
  add_file(key, file_path, ec);
  throw_error(ec, "add_file");
}

void form::add_file(const std::string& key, const std::string& file_path,
                    std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_FILE,
      file_path.c_str(), native::CURLFORM_END))};
}

void form::add_file(const std::string& key, const std::string& file_path,
                    const std::string& content_type) {
  std::error_code ec;
  add_file(key, file_path, content_type, ec);
  throw_error(ec, "add_file");
}

void form::add_file(const std::string& key, const std::string& file_path,
                    const std::string& content_type, std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_FILE,
      file_path.c_str(), native::CURLFORM_CONTENTTYPE, content_type.c_str(),
      native::CURLFORM_END))};
}

void form::add_file_using_name(const std::string& key,
                               const std::string& file_path,
                               const std::string& file_name) {
  std::error_code ec;
  add_file_using_name(key, file_path, file_name, ec);
  throw_error(ec, "add_file_using_name");
}

void form::add_file_using_name(const std::string& key,
                               const std::string& file_path,
                               const std::string& file_name,
                               std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_FILE,
      file_path.c_str(), native::CURLFORM_FILENAME, file_name.c_str(),
      native::CURLFORM_END))};
}

void form::add_file_using_name(const std::string& key,
                               const std::string& file_path,
                               const std::string& file_name,
                               const std::string& content_type) {
  std::error_code ec;
  add_file_using_name(key, file_path, file_name, content_type, ec);
  throw_error(ec, "add_file_using_name");
}

void form::add_file_using_name(const std::string& key,
                               const std::string& file_path,
                               const std::string& file_name,
                               const std::string& content_type,
                               std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_FILE,
      file_path.c_str(), native::CURLFORM_FILENAME, file_name.c_str(),
      native::CURLFORM_CONTENTTYPE, content_type.c_str(),
      native::CURLFORM_END))};
}

void form::add_file_content(const std::string& key,
                            const std::string& file_path) {
  std::error_code ec;
  add_file_content(key, file_path, ec);
  throw_error(ec, "add_file_content");
}

void form::add_file_content(const std::string& key,
                            const std::string& file_path, std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_FILECONTENT,
      file_path.c_str(), native::CURLFORM_END))};
}

void form::add_file_content(const std::string& key,
                            const std::string& file_path,
                            const std::string& content_type) {
  std::error_code ec;
  add_file_content(key, file_path, content_type, ec);
  throw_error(ec, "add_file_content");
}

void form::add_file_content(const std::string& key,
                            const std::string& file_path,
                            const std::string& content_type,
                            std::error_code& ec) {
  ec = std::error_code{static_cast<errc::FormErrorCode>(native::curl_formadd(
      &post_, &last_, native::CURLFORM_COPYNAME, key.c_str(),
      native::CURLFORM_NAMELENGTH, key.length(), native::CURLFORM_FILECONTENT,
      file_path.c_str(), native::CURLFORM_CONTENTTYPE, content_type.c_str(),
      native::CURLFORM_END))};
}

}  // namespace curl

USERVER_NAMESPACE_END
