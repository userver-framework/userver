#pragma once

/// @file userver/congestion_control/config.hpp
/// @brief Congestion Control config structures

#include <cstddef>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

struct Policy {
  size_t min_limit{2};
  double up_rate_percent{2};
  double down_rate_percent{5};

  size_t overload_on{10};
  size_t overload_off{3};

  size_t up_count{3};
  size_t down_count{3};
  size_t no_limit_count{1000};

  size_t load_limit_percent{0};
  size_t load_limit_crit_percent{0};

  double start_limit_factor{0.75};
};

Policy Parse(const formats::json::Value& policy, formats::parse::To<Policy>);

namespace impl {

struct RpsCcConfig {
  Policy policy;
  bool is_enabled{};
  int activate_factor{0};

  static RpsCcConfig Parse(const dynamic_config::DocsMap& docs_map);
};

inline constexpr dynamic_config::Key<RpsCcConfig::Parse> kRpsCcConfig;

}  // namespace impl

}  // namespace congestion_control

USERVER_NAMESPACE_END
