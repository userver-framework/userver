#pragma once

/// @file userver/storages/rocks/impl/exception.hpp
/// @brief rocks-specific exceptions

#include <stdexcept>
#include <string_view>

namespace rocksdb {
class Status;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

namespace detail {
void CheckStatus(const rocksdb::Status& status, std::string_view description);
}  // namespace detail

class StatusNokException : public std::runtime_error {
public:
    StatusNokException(std::string_view description, std::string_view status);
    std::string_view GetStatusString() const;

private:
    std::string_view status_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
