#include <string>
#include <rocksdb/db.h>


namespace storages::rocks {

    class Client {
    public:
        Client(const std::string& db_path);
        
        bool Put(const std::string& key, const std::string& value);

        bool Get(const std::string& key, std::string* value);

        bool Delete(const std::string& key);

        virtual ~Client() = default;

    private:
        rocksdb::DB* db_ = nullptr;
    };
}