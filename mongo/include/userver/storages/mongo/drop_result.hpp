#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

/// MongoDB drop operation result
class DropResult {
 public:
  DropResult(bool mongo_driver_result) : is_drop_succes_(mongo_driver_result) {}

  /// Is drop operation seccesfuly done
  bool IsSucces() const { return is_drop_succes_; }

  /// Is drop operation failed
  bool IsFailed() const { return !is_drop_succes_; }

 private:
  bool is_drop_succes_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
