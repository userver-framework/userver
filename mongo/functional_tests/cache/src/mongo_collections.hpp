#pragma once

#include <memory>
#include <string_view>
#include <type_traits>

#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/pool.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace functional_tests {

/// @brief The component that owns the mongo collections of a service is
/// generated outside of userver, so the test service defines its own.
class MongoCollections final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "mongo-collections";

    struct Collections {
        storages::mongo::Collection cached_documents;
    };

    MongoCollections(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context),
          collections_(std::make_shared<Collections>(Collections{
              context.FindComponent<components::Mongo>("cache-database").GetPool()->GetCollection("cached_documents"),
          }))
    {}

    template <typename T>
    std::shared_ptr<T> GetCollectionForLibrary() const {
        static_assert(std::is_same_v<T, Collections>);
        return collections_;
    }

private:
    const std::shared_ptr<Collections> collections_;
};

}  // namespace functional_tests
