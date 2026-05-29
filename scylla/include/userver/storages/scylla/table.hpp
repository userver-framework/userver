#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <userver/storages/scylla/operations.hpp>
#include <userver/storages/scylla/row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace impl {
class TableImpl;
}

struct PagedRows {
    Rows rows;
    std::string paging_state;
    bool has_more_pages{false};
};

class Table {
public:
    explicit Table(std::shared_ptr<impl::TableImpl>);

    const std::string& GetTableName() const;

    void Execute(const operations::InsertOne&);
    Row Execute(const operations::SelectOne&);
    Rows Execute(const operations::SelectMany&);
    void Execute(const operations::DeleteOne&);
    void Execute(const operations::UpdateOne&);
    std::int64_t Execute(const operations::Count&);
    void Execute(const operations::InsertMany&);
    void Execute(const operations::Truncate&);

    PagedRows ExecutePaged(const operations::SelectMany&, std::string paging_state = {});

    operations::LwtResult ExecuteLwt(const operations::InsertOne&);
    operations::LwtResult ExecuteLwt(const operations::UpdateOne&);
    operations::LwtResult ExecuteLwt(const operations::DeleteOne&);

private:
    std::shared_ptr<impl::TableImpl> impl_;
};

}  // namespace storages::scylla

USERVER_NAMESPACE_END
