#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/optional.hpp>
#include <bsoncxx/array/element.hpp>
#include <bsoncxx/array/value.hpp>
#include <bsoncxx/array/view.hpp>
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/oid.hpp>

#include <utils/flags.hpp>

namespace storages {
namespace mongo {

using DocumentValue = bsoncxx::document::value;
using DocumentElement = bsoncxx::document::element;
using ArrayValue = bsoncxx::array::value;
using ArrayElement = bsoncxx::array::element;

static const DocumentValue kEmptyObject(bsoncxx::document::view{});
static const ArrayValue kEmptyArray(bsoncxx::array::view{});

enum class SortOrder { kDescending = -1, kAscending = 1 };

class MongoError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class BadType : public MongoError {
 public:
  BadType(const DocumentElement& item, const char* context);
  BadType(const ArrayElement& item, const char* context);
};

class QueryError : public MongoError {
  using MongoError::MongoError;
};

enum class ElementKind {
  kNone = 0,
  kMissing = 1 << 0,
  kNull = 1 << 1,
  kBool = 1 << 2,
  kNumber = 1 << 3,
  kString = 1 << 4,
  kTimestamp = 1 << 5,
  kArray = 1 << 6,
  kDocument = 1 << 7,
  kOid = 1 << 8,
};

bool IsOneOf(const DocumentElement&, utils::Flags<ElementKind>);

double ToDouble(const DocumentElement&);
int64_t ToInt64(const DocumentElement&);
std::chrono::system_clock::time_point ToTimePoint(const DocumentElement&);
bsoncxx::oid ToOid(const DocumentElement&);
std::string ToString(const DocumentElement&);
bool ToBool(const DocumentElement&);
bsoncxx::array::view ToArray(const DocumentElement&);
bsoncxx::document::view ToDocument(const DocumentElement&);
std::vector<std::string> ToStringArray(const DocumentElement&);

bool IsOneOf(const ArrayElement&, utils::Flags<ElementKind>);

double ToDouble(const ArrayElement&);
int64_t ToInt64(const ArrayElement&);
std::chrono::system_clock::time_point ToTimePoint(const ArrayElement&);
bsoncxx::oid ToOid(const ArrayElement&);
std::string ToString(const ArrayElement&);
bool ToBool(const ArrayElement&);
bsoncxx::array::view ToArray(const ArrayElement&);
bsoncxx::document::view ToDocument(const ArrayElement&);
std::vector<std::string> ToStringArray(const ArrayElement&);

namespace impl {

template <typename T>
struct ConversionTraits {};

template <>
struct ConversionTraits<double> {
  static double Convert(const DocumentElement& item) { return ToDouble(item); }
  static double Convert(const ArrayElement& item) { return ToDouble(item); }
};

template <>
struct ConversionTraits<bool> {
  static bool Convert(const DocumentElement& item) { return ToBool(item); }
  static bool Convert(const ArrayElement& item) { return ToBool(item); }
};

}  // namespace impl

template <typename T>
boost::optional<T> ToOptional(const DocumentElement& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) {
    return {};
  }
  return impl::ConversionTraits<T>::Convert(item);
}

template <typename T>
boost::optional<T> ToOptional(const ArrayElement& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) {
    return {};
  }
  return impl::ConversionTraits<T>::Convert(item);
}

DocumentValue Or(const std::vector<bsoncxx::document::view_or_value>& docs);

}  // namespace mongo
}  // namespace storages
