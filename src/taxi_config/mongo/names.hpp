#pragma once

#include <string>

namespace mongo {
namespace db {

namespace taxi {
static const std::string kDb = "dbtaxi";
static const std::string kDbProcessing = "dbprocessing";

namespace config {
static const std::string kCollection = kDb + ".config";

static const std::string kId = "_id";
static const std::string kUpdated = "updated";
}  // namespace config

}  // namespace taxi
}  // namespace db
}  // namespace mongo
