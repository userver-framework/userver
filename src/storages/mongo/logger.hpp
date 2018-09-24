#pragma once

#include <mongocxx/logger.hpp>

namespace storages {
namespace mongo {
namespace impl {

class Logger : public mongocxx::logger {
 public:
  void operator()(mongocxx::log_level level, mongocxx::stdx::string_view domain,
                  mongocxx::stdx::string_view message) noexcept override;
};

}  // namespace impl
}  // namespace mongo
}  // namespace storages
