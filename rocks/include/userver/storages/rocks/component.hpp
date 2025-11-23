#pragma once

/// @file userver/storages/rocks/component.hpp
/// @brief @copybrief components::Rocks

#include <string_view>
#include <userver/storages/rocks/db_fwd.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/**
 * @brief Component for configuring and managing RocksDB.
 */
class Rocks final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "rocks";
    static yaml_config::Schema GetStaticConfigSchema();

    /**
     * @brief Constructor of the Rocks class.
     */
    Rocks(const components::ComponentConfig&, const components::ComponentContext&);

    /**
     * @brief Return a pointer to the database instance.
     */
    [[nodiscard]] const storages::rocks::DbPtr& GetDb() const;

private:
    storages::rocks::DbPtr db_ptr_;
};

}  // namespace components

USERVER_NAMESPACE_END
