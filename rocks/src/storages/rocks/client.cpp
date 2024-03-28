#include <userver/storages/rocks/client.hpp>

namespace storages::rocks {

    Client::Client(const std::string& db_path) {
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
        if (!status.ok()) {
            throw "some error";
        }
    }

    bool Client::Put(const std::string& key, const std::string& value) {
        rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
        return status.ok();
    }

    bool Client::Get(const std::string& key, std::string* value) {
        rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, value);
        return status.ok();
    }


    bool Client::Delete(const std::string& key) {
        rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
        return status.ok();
    }

}