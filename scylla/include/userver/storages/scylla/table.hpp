#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <userver/storages/scylla/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace impl {
class TableImpl;
}

class Table {
public:
    explicit Table(std::shared_ptr<impl::TableImpl>);

    const std::string& GetTableName() const;

    void Execute(const operations::InsertOne&);

    operations::SelectOne::Row Execute(const operations::SelectOne&);

    operations::SelectMany::ResultSet Execute(const operations::SelectMany&);

    void Execute(const operations::DeleteOne&);

    void Execute(const operations::UpdateOne&);

    int64_t Execute(const operations::Count&);

    void Execute(const operations::InsertMany&);

    void Execute(const operations::Truncate&);

private:
    std::shared_ptr<impl::TableImpl> impl_;
};

}

USERVER_NAMESPACE_END
