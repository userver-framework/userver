#pragma once

#include <userver/formats/bson/document.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class CursorImpl {
 public:
  virtual ~CursorImpl() = default;

  virtual bool IsValid() const = 0;
  virtual bool HasMore() const = 0;

  virtual const formats::bson::Document& Current() const = 0;
  virtual void Next() = 0;
};

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
