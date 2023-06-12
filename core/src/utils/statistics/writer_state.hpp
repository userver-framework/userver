#pragma once

#include <vector>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

struct WriterState {
  BaseFormatBuilder& builder;
  const Request& request;
  std::string path;
  std::vector<LabelView> add_labels;
};

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
