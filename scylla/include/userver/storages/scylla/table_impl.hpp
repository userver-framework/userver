#pragma once

#include <cstdint>
#include <string>

#include <userver/storages/scylla/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl {

class TableImpl {
public:
    virtual ~TableImpl() = default;

    const std::string& GetKeyspaceName() const;
    const std::string& GetTableName() const;

    virtual void Execute(const operations::InsertOne&) = 0;

    virtual operations::SelectOne::Row Execute(const operations::SelectOne&) = 0;

    virtual operations::SelectMany::ResultSet Execute(const operations::SelectMany&) = 0;

    virtual void Execute(const operations::DeleteOne&) = 0;

    virtual void Execute(const operations::UpdateOne&) = 0;

    virtual int64_t Execute(const operations::Count&) = 0;

    virtual void Execute(const operations::InsertMany&) = 0;

    virtual void Execute(const operations::Truncate&) = 0;

protected:
    TableImpl(std::string keyspace_name, std::string table_name);

private:
    const std::string keyspace_name_;
    const std::string table_name_;
};

}

USERVER_NAMESPACE_END
