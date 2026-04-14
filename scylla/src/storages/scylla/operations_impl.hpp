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

class DeleteOne::Impl {
public:
    std::vector<DeleteOne::Condition> conditions;
};

class SelectMany::Impl {
public:
    std::vector<std::string> columns;
    bool select_all{true};
    std::vector<SelectMany::Condition> conditions;
    size_t limit{0};
};

class UpdateOne::Impl {
public:
    std::vector<UpdateOne::Assignment> assignments;
    std::vector<UpdateOne::Condition> conditions;
};

class Count::Impl {
public:
    std::vector<Count::Condition> conditions;
};

class InsertMany::Impl {
public:

    std::vector<std::vector<InsertMany::Binding>> rows{{}};
};

}

USERVER_NAMESPACE_END
