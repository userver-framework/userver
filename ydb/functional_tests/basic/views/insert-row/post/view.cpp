#include "view.hpp"

#include <userver/formats/json.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/table.hpp>

namespace {

const ydb::Query kInsertQuery{
    R"(
--!syntax_v1
DECLARE $id_key AS String;
DECLARE $name_key AS Utf8;
DECLARE $service_key AS String;
DECLARE $channel_key AS Int64;
DECLARE $state_key AS Json?;

INSERT INTO events (id, name, service, channel, created, state)
VALUES ($id_key, $name_key, $service_key, $channel_key, CurrentUtcTimestamp(), $state_key);
    )",
    ydb::Query::Name{"insert-row"},
};

}  // namespace

namespace sample {

formats::json::Value
InsertRowHandler::HandleRequestJsonThrow(const server::http::HttpRequest&, const formats::json::Value& request, server::request::RequestContext&)
    const {
    try {
        Ydb().RetryTx("trx", {.tx_mode = ydb::TransactionMode::kSerializableRW}, [&](ydb::TxActor& tx) {
            auto response = tx.Execute(
                kInsertQuery,  //
                "$id_key",
                request["id"].As<std::string>(),  //
                "$name_key",
                ydb::Utf8{request["name"].As<std::string>()},  //
                "$service_key",
                request["service"].As<std::string>(),  //
                "$channel_key",
                request["channel"].As<int64_t>(),  //
                "$state_key",
                request["state"].As<std::optional<formats::json::Value>>()  //
            );

            if (response.GetCursorCount()) {
                throw std::runtime_error("Unexpected response data");
            }

            return ydb::TxAction::kCommit;
        });
    } catch (const ydb::YdbResponseError& ex) {
        if (ex.IsConstraintViolation()) {
            throw server::handlers::CustomHandlerException(server::handlers::HandlerErrorCode::kConflictState);
        }
        throw;
    }

    return formats::json::MakeObject();
}

}  // namespace sample
