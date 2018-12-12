#pragma once

#include "component_base.hpp"

#include <components/manager.hpp>
#include <utils/statistics/storage.hpp>

namespace components {

class StatisticsStorage : public LoggableComponentBase {
 public:
  static constexpr auto kName = "statistics-storage";

  StatisticsStorage(const ComponentConfig& config,
                    const components::ComponentContext&);

  ~StatisticsStorage();

  utils::statistics::Storage& GetStorage() { return storage_; }

  const utils::statistics::Storage& GetStorage() const { return storage_; }

 private:
  utils::statistics::Storage storage_;
};

}  // namespace components
