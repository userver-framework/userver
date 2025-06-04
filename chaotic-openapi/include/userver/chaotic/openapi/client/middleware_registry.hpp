#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <userver/chaotic/openapi/client/middleware_factory.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

class MiddlewareRegistry {
public:
    static MiddlewareRegistry& Instance() {
        static MiddlewareRegistry instance;
        return instance;
    }

    void Register(const std::string& name, std::unique_ptr<MiddlewareFactory> factory) {
        const std::lock_guard<std::mutex> lock(mutex_);
        factories_.emplace(name, std::move(factory));
    }

    const std::unordered_map<std::string, std::unique_ptr<MiddlewareFactory>>& GetFactories() const {
        const std::lock_guard<std::mutex> lock(mutex_);
        return factories_;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<MiddlewareFactory>> factories_;
};

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
