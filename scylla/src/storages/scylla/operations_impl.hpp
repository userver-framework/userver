#pragma once

#include <string>
#include <variant>
#include <vector>

#include <userver/storages/scylla/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::operations {

class InsertOne::Impl {
public:
    struct Binding {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    std::vector<Binding> bindings;
};

class SelectOne::Impl {
public:
    std::vector<std::string> columns;
    bool select_all{true};
    std::vector<SelectOne::Condition> conditions;
};

}  // namespace storages::scylla::operations

USERVER_NAMESPACE_END