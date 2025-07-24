#pragma once

#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

void GetRedisKey(const std::string& key, size_t* key_start, size_t* key_len);

class KeyShard {
public:
    virtual ~KeyShard() = default;
    virtual size_t ShardByKey(const std::string& key) const = 0;
};

class KeyShardFactory {
    std::string type_;

public:
    KeyShardFactory(const std::string& type) : type_(type) {}
    std::unique_ptr<KeyShard> operator()(size_t nshards);
    bool IsClusterStrategy() const;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
