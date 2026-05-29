#include <userver/storages/scylla/table_impl.hpp>

#include <userver/storages/scylla/exception.hpp>
#include <userver/utils/text.hpp>

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl {
TableImpl::TableImpl(std::string keyspace_name, std::string table_name)
    : keyspace_name_(keyspace_name),
      table_name_(table_name)
{
    if (!utils::text::IsCString(keyspace_name_)) {
        throw InvalidConfigException("keyspace name must be a string");
    }
    if (!utils::text::IsCString(table_name_)) {
        throw InvalidConfigException("table name must be a string");
    }
}

const std::string& TableImpl::GetKeyspaceName() const { return keyspace_name_; }
const std::string& TableImpl::GetTableName() const { return table_name_; }

}  // namespace storages::scylla::impl

USERVER_NAMESPACE_END
