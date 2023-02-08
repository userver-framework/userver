#pragma once

#include <memory>
#include <optional>

#include <userver/formats/bson/document.hpp>

#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/cursor_impl.hpp>
#include <storages/mongo/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

class CDriverCursorImpl final : public CursorImpl {
 public:
  CDriverCursorImpl(cdriver::CDriverPoolImpl::BoundClientPtr,
                    cdriver::CursorPtr,
                    std::shared_ptr<stats::OperationStatisticsItem> find_stats);

  bool IsValid() const override;
  bool HasMore() const override;

  const formats::bson::Document& Current() const override;
  void Next() override;

 private:
  std::optional<formats::bson::Document> current_;
  cdriver::CDriverPoolImpl::BoundClientPtr client_;
  cdriver::CursorPtr cursor_;
  const std::shared_ptr<stats::OperationStatisticsItem> find_stats_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
