#pragma once

// !TODO this @file curl-ev/mime.cpp create in 2025 year                    //

#include <memory>
#include <string>
#include <vector>

#include <curl-ev/native.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

class mime final {
public:
    mime(native::curl_mime* mime_ptr);
    ~mime();

    mime(const mime& rhs) = delete;
    mime(mime&& rhs) noexcept = default;

    mime& operator= (const mime& rhs) = delete;
    mime& operator= (mime&& rhs) noexcept = default;

    inline native::curl_mime*       native_mime() { return mime_; }
    inline native::curl_mimepart*   native_part() { return part_; }

private:
    native::curl_mime*      mime_ { nullptr };
    native::curl_mimepart*  part_ { nullptr };
};

} // namespace curl

USERVER_NAMESPACE_END