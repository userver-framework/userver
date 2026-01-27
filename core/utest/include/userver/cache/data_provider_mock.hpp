#pragma once

/// @file userver/cache/data_provider_mock.hpp
/// @brief Mock implementation of DataProvider

#include <userver/cache/data_provider.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// @brief Mock implementation of DataProvider for testing purposes
/// @tparam T The type of data being provided
///
/// DataProviderMock is a mock implementation of the DataProvider interface
/// that stores static data and returns shared readable pointers to it.
/// It's designed for use in unit tests where you want to provide
/// controlled data to components that depend on DataProvider.
template <typename T>
class DataProviderMock : public DataProvider<T> {
public:
    using DataProvider<T>::DataType;

    explicit DataProviderMock(DataType data)
        : data_{std::make_shared<DataType>(std::move(data))}
    {}
    explicit DataProviderMock(std::shared_ptr<DataType> data)
        : data_{std::move(data)}
    {}

    utils::SharedReadablePtr<DataType> Get() const final {
        return utils::SharedReadablePtr<DataType>(data_.ReadCopy());
    };

private:
    rcu::Variable<std::shared_ptr<const DataType>> data_;
};

}  // namespace cache

USERVER_NAMESPACE_END
