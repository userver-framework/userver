#include <string_view>
#include <userver/storages/rocks/db.hpp>
#include <userver/storages/rocks/detail/db_impl.hpp>
#include <userver/storages/rocks/write_batch.hpp>

#include <fmt/format.h>

#include <rocksdb/compression_type.h>
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/options.h>

namespace {

constexpr struct { std::string_view name; rocksdb::CompressionType type; } kCompressionTable[] = {
    {"no_compression", rocksdb::kNoCompression},
    {"lz4", rocksdb::kLZ4Compression},
    {"zstd", rocksdb::kZSTD}
};

rocksdb::CompressionType ParseCompressionType(std::string_view compression) {
    for (const auto& entry : kCompressionTable) {
        if (entry.name == compression) {
            return entry.type;
        }
    }
    throw std::runtime_error(fmt::format("Invalid compression type: {}", compression));
}

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Db::Db(const std::string& db_path, int max_background_jobs, const std::vector<std::string>& column_families,
        const DbOptions& db_options, engine::TaskProcessor& task_processor) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;
    options.max_background_jobs = max_background_jobs;
    if (db_options.compression) options.compression = ParseCompressionType(*db_options.compression);
    if (db_options.compression_level) options.compression_opts.level = *db_options.compression_level;
    if (db_options.bottommost_compression) {
        options.bottommost_compression = ParseCompressionType(*db_options.bottommost_compression);
    }
    if (db_options.bottommost_compression_level) {
        options.bottommost_compression_opts.level = *db_options.bottommost_compression_level;
    }
    if (db_options.use_direct_reads) {
        options.use_direct_reads = *db_options.use_direct_reads;
    }
    if (db_options.use_direct_io_for_flush_and_compaction) {
        options.use_direct_io_for_flush_and_compaction = *db_options.use_direct_io_for_flush_and_compaction;
    }
    db_impl_ = std::make_shared<detail::DbImpl>(options, db_path, column_families, task_processor);
}

ColumnFamilyHandle Db::GetColumnFamily(const std::string& name) const {
    return db_impl_->GetColumnFamily(name);
}

void Db::Put(std::string_view key, std::string_view value) {
    db_impl_->Put(rocksdb::WriteOptions{}, key, value);
}

void Db::Put(ColumnFamilyHandle column_family, std::string_view key, std::string_view value) {
    db_impl_->Put(rocksdb::WriteOptions{}, column_family, key, value);
}

void Db::Delete(std::string_view key) {
    db_impl_->Delete(rocksdb::WriteOptions{}, key);
}

void Db::Delete(ColumnFamilyHandle column_family, std::string_view key) {
    db_impl_->Delete(rocksdb::WriteOptions{}, column_family, key);
}

void Db::Write(WriteBatch& write_batch) {
    db_impl_->Write(rocksdb::WriteOptions{}, write_batch.GetPimpl());
}

std::optional<std::string> Db::Get(std::string_view key) const {
    return db_impl_->Get(rocksdb::ReadOptions{}, key);
}

std::optional<std::string> Db::Get(ColumnFamilyHandle column_family, std::string_view key) const {
    return db_impl_->Get(rocksdb::ReadOptions{}, column_family, key);
}

Snapshot Db::GetSnapshot() const {
    return Snapshot{
        detail::SnapshotImpl{
            db_impl_, db_impl_->GetSnapshot()
        }
    };
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
