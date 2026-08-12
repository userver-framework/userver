#pragma once

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace s3api {

class AuthHeaderConflictError : public std::runtime_error {
    using runtime_error::runtime_error;
};

class NoBucketError : public std::runtime_error {
    using runtime_error::runtime_error;
};

class ListBucketError : public std::runtime_error {
    using runtime_error::runtime_error;
};

class ResponseParsingError : public std::runtime_error {
    using runtime_error::runtime_error;
};

class MultipartUploadError : public std::runtime_error {
    using runtime_error::runtime_error;
};

}  // namespace s3api

USERVER_NAMESPACE_END
