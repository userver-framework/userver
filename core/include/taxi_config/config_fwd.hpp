#pragma once

namespace taxi_config {

template <typename ConfigTag>
class BaseConfig;

struct FullConfigTag;

using Config = BaseConfig<FullConfigTag>;

}  // namespace taxi_config
