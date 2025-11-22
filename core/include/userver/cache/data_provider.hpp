#pragma once

/// @file userver/cache/data_provider.hpp
/// @brief A simple interface for hiding implementation and making mocks easy to implement

#include <userver/utils/shared_readable_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// @brief Interface for providing cached data of type T
/// @tparam T The type of data being provided
///
/// DataProvider is a simple interface that allows hiding implementation
/// details and making mocks easy to implement. It provides a way to
/// retrieve shared readable pointers to cached data.

template <typename T>
class DataProvider {
public:
    using DataType = T;

    virtual ~DataProvider() = default;

    virtual utils::SharedReadablePtr<DataType> Get() const = 0;
};

}  // namespace cache

USERVER_NAMESPACE_END
