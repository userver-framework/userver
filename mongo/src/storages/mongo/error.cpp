#include <storages/mongo/error.hpp>

#include <string>

#include <bsoncxx/types.hpp>

#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {
namespace {

std::string MakeElementDescription(const DocumentElement& item) {
  return "item '" + item.key().to_string() + '\'';
}

std::string MakeElementDescription(const ArrayElement&) { return "array item"; }

template <typename Element>
std::string MakeBadTypeDescription(const Element& item, const char* context) {
  if (!item) {
    return std::string("item is missing for ") + context;
  }
  return MakeElementDescription(item) + " has type" + to_string(item.type()) +
         " (" +
         std::to_string(
             static_cast<std::underlying_type_t<bsoncxx::type>>(item.type())) +
         "), which is unsuitable for " + context;
}

}  // namespace

BadType::BadType(const DocumentElement& item, const char* context)
    : MongoError(MakeBadTypeDescription(item, context)) {}

BadType::BadType(const ArrayElement& item, const char* context)
    : MongoError(MakeBadTypeDescription(item, context)) {}

}  // namespace mongo
}  // namespace storages
