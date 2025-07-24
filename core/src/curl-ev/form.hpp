/** @file curl-ev/form.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for constructing libcurl forms
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/utils/zstring_view.hpp>

#include "native.hpp"

USERVER_NAMESPACE_BEGIN

namespace curl {

class form {
public:
    form();
    form(const form&) = delete;
    form(form&&) = delete;
    form& operator=(const form&) = delete;
    form& operator=(form&&) = delete;
    ~form();

    inline native::curl_httppost* native_handle() { return post_; };

    void add_content(std::string_view key, std::string_view content);
    void add_content(std::string_view key, std::string_view content, std::error_code& ec);
    void add_content(std::string_view key, std::string_view content, utils::zstring_view content_type);
    void
    add_content(std::string_view key, std::string_view content, utils::zstring_view content_type, std::error_code& ec);

    void add_buffer(std::string_view key, utils::zstring_view file_name, const std::shared_ptr<std::string>& buffer);
    void add_buffer(
        std::string_view key,
        utils::zstring_view file_name,
        const std::shared_ptr<std::string>& buffer,
        std::error_code& ec
    );
    void add_buffer(
        std::string_view key,
        utils::zstring_view file_name,
        const std::shared_ptr<std::string>& buffer,
        utils::zstring_view content_type
    );
    void add_buffer(
        std::string_view key,
        utils::zstring_view file_name,
        const std::shared_ptr<std::string>& buffer,
        utils::zstring_view content_type,
        std::error_code& ec
    );

private:
    void add_buffer(
        std::string_view key,
        utils::zstring_view file_name,
        const char* buffer,
        size_t buffer_len,
        std::error_code& ec
    );

    native::curl_httppost* post_{nullptr};
    native::curl_httppost* last_{nullptr};
    std::vector<std::shared_ptr<std::string>> buffers_;
};

}  // namespace curl

USERVER_NAMESPACE_END
