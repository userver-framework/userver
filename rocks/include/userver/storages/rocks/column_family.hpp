#pragma once

namespace rocksdb {
class ColumnFamilyHandle;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

using ColumnFamilyHandle = rocksdb::ColumnFamilyHandle*;

}  // namespace storages::rocks

USERVER_NAMESPACE_END
