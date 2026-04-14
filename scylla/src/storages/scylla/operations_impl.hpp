#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <userver/storages/scylla/operations.hpp>
#include <userver/storages/scylla/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::operations {

struct NamedValue {
    std::string column_name;
    Value value;
};

struct UsingClause {
    std::optional<std::int32_t> ttl_seconds;
    std::optional<std::int64_t> timestamp_micros;
};

class InsertOne::Impl {
public:
    std::vector<NamedValue> bindings;
    UsingClause using_clause{};
    bool if_not_exists{false};
};

class SelectOne::Impl {
public:
    std::vector<std::string> columns;
    std::vector<NamedValue> conditions;
    bool select_all{true};
};

class DeleteOne::Impl {
public:
    std::vector<NamedValue> conditions;
    std::vector<NamedValue> if_conditions;
    bool if_exists{false};
};

class SelectMany::Impl {
public:
    std::vector<std::string> columns;
    std::vector<NamedValue> conditions;
    std::size_t limit{0};
    std::size_t page_size{0};
    bool select_all{true};
    bool allow_filtering{false};
};

class UpdateOne::Impl {
public:
    std::vector<NamedValue> assignments;
    std::vector<NamedValue> conditions;
    std::vector<NamedValue> if_conditions;
    UsingClause using_clause{};
    bool if_exists{false};
};

class Count::Impl {
public:
    std::vector<NamedValue> conditions;
};

class InsertMany::Impl {
public:
    std::vector<std::vector<NamedValue>> rows{{}};
    UsingClause using_clause{};
};

}  // namespace storages::scylla::operations

USERVER_NAMESPACE_END
