#pragma once

#include <userver/storages/postgres/query.hpp>

namespace uservice_dynconf::sql {

const storages::postgres::Query kSelectUser{
    "SELECT username, nonce, timestamp, nonce_count, ha1 "
    "FROM auth_schema.users WHERE username=$1",
    storages::postgres::Query::Name{"select_user"}};

const storages::postgres::Query kUpdateUser{
    "UPDATE auth_schema.users "
    "SET nonce=$1, timestamp=$2, nonce_count=$3 "
    "WHERE username=$4",
    storages::postgres::Query::Name{"update_user"}};

/// [insert unnamed nonce]
/// 1) Searches for id of expired nonce or generates new nonce.
/// 2) Insert nonce and it's creation time.
/// if id is selected from present nonces
/// then there will be conflict
/// so we rewrite nonce information.
/// 3) Otherwise new nonce will be inserted.
/// Purpose is not storing expired nonces and making deleting query after
/// every query
const storages::postgres::Query kInsertUnnamedNonce{
    "WITH expired AS( "
    "  SELECT id FROM auth_schema.unnamed_nonce WHERE creation_time <= $1 "
    "LIMIT 1 "
    "), "
    "free_id AS ( "
    "SELECT COALESCE((SELECT id FROM expired LIMIT 1), "
    "uuid_generate_v4()) AS id "
    ") "
    "INSERT INTO auth_schema.unnamed_nonce (id, nonce, creation_time) "
    "SELECT "
    "  free_id.id, "
    "  $2, "
    "  $3 "
    "FROM free_id "
    "ON CONFLICT (id) DO UPDATE SET "
    "  nonce=$2, "
    "  creation_time=$3 "
    "  WHERE auth_schema.unnamed_nonce.id=(SELECT free_id.id FROM free_id "
    "LIMIT 1) ",
    storages::postgres::Query::Name{"insert_unnamed_nonce"}};
/// [insert unnamed nonce]
const storages::postgres::Query kSelectUnnamedNonce{
    "UPDATE auth_schema.unnamed_nonce SET nonce=id WHERE nonce=$1 "
    "RETURNING creation_time ",
    storages::postgres::Query::Name{"select_unnamed_nonce"}};

}  // namespace uservice_dynconf::sql
