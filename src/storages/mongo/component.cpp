#include <storages/mongo/component.hpp>

#include <stdexcept>

#include <mongo/client/init.h>

#include <logging/log.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/exceptions.hpp>
#include "mongo_secdist.hpp"

namespace components {

namespace {

class MongoClientKeeper {
 public:
  MongoClientKeeper() : status_(mongo::ErrorCodes::OK, {}) {
    mongo::client::Options options;
    options.setIPv6Enabled(true);
    status_ = mongo::client::initialize(options);
  }

  ~MongoClientKeeper() {
    const auto status = mongo::client::shutdown();
    if (!status.isOK()) {
      LOG_ERROR() << "Error during mongo lib shutdown: " << status.toString();
    }
  }

  MongoClientKeeper(const MongoClientKeeper&) = delete;
  MongoClientKeeper(MongoClientKeeper&&) = delete;
  MongoClientKeeper& operator=(const MongoClientKeeper&) = delete;
  MongoClientKeeper& operator=(MongoClientKeeper&&) = delete;

  void Check() const {
    if (!status_.isOK()) {
      throw std::runtime_error("Failed to initialize mongo lib: " +
                               status_.toString());
    }
  }

 private:
  mongo::Status status_;
};

}  // namespace

Mongo::Mongo(const ComponentConfig& config, const ComponentContext& context) {
  auto dbalias = config.ParseString("dbalias", {});

  std::string connection_string;
  if (!dbalias.empty()) {
    try {
      auto* secdist = context.FindComponent<Secdist>();
      if (!secdist)
        throw std::runtime_error("Mongo requires secdist component");
      connection_string = secdist->Get()
                              .Get<storages::mongo::secdist::MongoSettings>()
                              .GetConnectionString(dbalias);
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load mongo config for dbalias " << dbalias
                  << ": " << ex.what();
      throw;
    }
  } else {
    connection_string = config.ParseString("dbconnection");
  }

  const auto so_timeout_ms =
      config.ParseInt("so_timeout_ms", kDefaultSoTimeoutMs);
  const auto min_pool_size =
      config.ParseUint64("min_pool_size", kDefaultMinPoolSize);
  const auto max_pool_size =
      config.ParseUint64("max_pool_size", kDefaultMaxPoolSize);

  // legacy driver
  static MongoClientKeeper mongo_client_keeper;
  mongo_client_keeper.Check();

  auto task_processor_name = config.ParseString("task_processor");
  auto* task_processor = context.GetTaskProcessor(task_processor_name);
  if (!task_processor) {
    throw std::runtime_error("Cannot find task processor with name '" +
                             task_processor_name + '\'');
  }

  pool_ = std::make_shared<storages::mongo::Pool>(
      connection_string, *task_processor, so_timeout_ms, min_pool_size,
      max_pool_size);
}

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

}  // namespace components
