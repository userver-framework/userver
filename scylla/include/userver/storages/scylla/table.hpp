#pragma once

#include <memory>
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

private:
    std::shared_ptr<impl::TableImpl> impl_;
};

}  // namespace storages::scylla

USERVER_NAMESPACE_END