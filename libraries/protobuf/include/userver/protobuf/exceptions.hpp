#pragma once

/// @file userver/protobuf/exceptions.hpp
/// @brief Exceptions thrown by the library.

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for the userver protobuf library.
namespace protobuf {

/// @brief Library base exception type.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace protobuf

USERVER_NAMESPACE_END
