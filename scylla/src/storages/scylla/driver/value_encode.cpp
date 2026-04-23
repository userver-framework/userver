#include "value_encode.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

#include <fmt/format.h>

#include <cassandra.h>

#include <userver/storages/scylla/exception.hpp>
#include <userver/utils/overloaded.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

void CheckRc(CassError rc, std::string_view what) {
    if (rc != CASS_OK) {
        throw QueryException(fmt::format("{}: {}", what, cass_error_desc(rc)));
    }
}

CassUuid ToCassUuid(const Uuid& u) noexcept {
    CassUuid out;
    out.time_and_version = u.time_and_version;
    out.clock_seq_and_node = u.clock_seq_and_node;
    return out;
}

CassInet ParseCassInet(const Inet& inet) {
    CassInet out;
    if (cass_inet_from_string(inet.text.c_str(), &out) != CASS_OK) {
        throw InvalidQueryArgumentException(fmt::format("Invalid inet '{}'", inet.text));
    }
    return out;
}

cass_int64_t TimestampToMs(const Timestamp& ts) noexcept {
    return static_cast<
        cass_int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()).count());
}

CassCollectionPtr BuildList(const List& list);
CassCollectionPtr BuildSet(const Set& set);
CassCollectionPtr BuildMap(const Map& map);

class StatementWriter {
public:
    StatementWriter(CassStatement* stmt, std::size_t idx) noexcept : stmt_{stmt}, idx_{idx} {}

    void WriteNull() { CheckRc(cass_statement_bind_null(stmt_, idx_), "bind_null"); }
    void WriteBool(bool v) { CheckRc(cass_statement_bind_bool(stmt_, idx_, v ? cass_true : cass_false), "bind_bool"); }
    void WriteInt8(std::int8_t v) { CheckRc(cass_statement_bind_int8(stmt_, idx_, v), "bind_int8"); }
    void WriteInt16(std::int16_t v) { CheckRc(cass_statement_bind_int16(stmt_, idx_, v), "bind_int16"); }
    void WriteInt32(std::int32_t v) { CheckRc(cass_statement_bind_int32(stmt_, idx_, v), "bind_int32"); }
    void WriteInt64(std::int64_t v) { CheckRc(cass_statement_bind_int64(stmt_, idx_, v), "bind_int64"); }
    void WriteFloat(float v) { CheckRc(cass_statement_bind_float(stmt_, idx_, v), "bind_float"); }
    void WriteDouble(double v) { CheckRc(cass_statement_bind_double(stmt_, idx_, v), "bind_double"); }
    void WriteString(std::string_view v) {
        CheckRc(cass_statement_bind_string_n(stmt_, idx_, v.data(), v.size()), "bind_string");
    }
    void WriteUuid(const Uuid& v) { CheckRc(cass_statement_bind_uuid(stmt_, idx_, ToCassUuid(v)), "bind_uuid"); }
    void WriteTimestamp(const Timestamp& v) {
        CheckRc(cass_statement_bind_int64(stmt_, idx_, TimestampToMs(v)), "bind_timestamp");
    }
    void WriteDate(const Date& v) { CheckRc(cass_statement_bind_uint32(stmt_, idx_, v.days_since_epoch), "bind_date"); }
    void WriteTime(const Time& v) {
        CheckRc(cass_statement_bind_int64(stmt_, idx_, v.nanoseconds_of_day), "bind_time");
    }
    void WriteBlob(const Blob& v) {
        CheckRc(
            cass_statement_bind_bytes(stmt_, idx_, reinterpret_cast<const cass_byte_t*>(v.data()), v.size()),
            "bind_bytes"
        );
    }
    void WriteInet(const Inet& v) { CheckRc(cass_statement_bind_inet(stmt_, idx_, ParseCassInet(v)), "bind_inet"); }
    void WriteList(const List& v) {
        const auto col = BuildList(v);
        CheckRc(cass_statement_bind_collection(stmt_, idx_, col.get()), "bind_list");
    }
    void WriteSet(const Set& v) {
        const auto col = BuildSet(v);
        CheckRc(cass_statement_bind_collection(stmt_, idx_, col.get()), "bind_set");
    }
    void WriteMap(const Map& v) {
        const auto col = BuildMap(v);
        CheckRc(cass_statement_bind_collection(stmt_, idx_, col.get()), "bind_map");
    }

private:
    CassStatement* stmt_;
    std::size_t idx_;
};

class CollectionWriter {
public:
    explicit CollectionWriter(CassCollection* col) noexcept : col_{col} {}

    [[noreturn]] void WriteNull() const { throw QueryException("Cannot append NULL inside a CQL collection"); }
    [[noreturn]] void WriteList(const List&) const { ThrowNestedCollection(); }
    [[noreturn]] void WriteSet(const Set&) const { ThrowNestedCollection(); }
    [[noreturn]] void WriteMap(const Map&) const { ThrowNestedCollection(); }

    void WriteBool(bool v) {
        CheckRc(cass_collection_append_bool(col_, v ? cass_true : cass_false), "collection.append_bool");
    }
    void WriteInt8(std::int8_t v) { CheckRc(cass_collection_append_int8(col_, v), "collection.append_int8"); }
    void WriteInt16(std::int16_t v) { CheckRc(cass_collection_append_int16(col_, v), "collection.append_int16"); }
    void WriteInt32(std::int32_t v) { CheckRc(cass_collection_append_int32(col_, v), "collection.append_int32"); }
    void WriteInt64(std::int64_t v) { CheckRc(cass_collection_append_int64(col_, v), "collection.append_int64"); }
    void WriteFloat(float v) { CheckRc(cass_collection_append_float(col_, v), "collection.append_float"); }
    void WriteDouble(double v) { CheckRc(cass_collection_append_double(col_, v), "collection.append_double"); }
    void WriteString(std::string_view v) {
        CheckRc(cass_collection_append_string_n(col_, v.data(), v.size()), "collection.append_string");
    }
    void WriteUuid(const Uuid& v) {
        CheckRc(cass_collection_append_uuid(col_, ToCassUuid(v)), "collection.append_uuid");
    }
    void WriteTimestamp(const Timestamp& v) {
        CheckRc(cass_collection_append_int64(col_, TimestampToMs(v)), "collection.append_timestamp");
    }
    void WriteDate(const Date& v) {
        CheckRc(cass_collection_append_uint32(col_, v.days_since_epoch), "collection.append_date");
    }
    void WriteTime(const Time& v) {
        CheckRc(cass_collection_append_int64(col_, v.nanoseconds_of_day), "collection.append_time");
    }
    void WriteBlob(const Blob& v) {
        CheckRc(
            cass_collection_append_bytes(col_, reinterpret_cast<const cass_byte_t*>(v.data()), v.size()),
            "collection.append_bytes"
        );
    }
    void WriteInet(const Inet& v) {
        CheckRc(cass_collection_append_inet(col_, ParseCassInet(v)), "collection.append_inet");
    }

private:
    [[noreturn]] static void ThrowNestedCollection() {
        throw QueryException("Nested collections inside collections are not supported");
    }

    CassCollection* col_;
};

template <typename Writer>
void EncodeValue(Writer& writer, const Value& value) {
    utils::Visit(
        value.Raw(),
        [&](std::monostate) { writer.WriteNull(); },
        [&](const std::string& v) { writer.WriteString(v); },
        [&](std::int8_t v) { writer.WriteInt8(v); },
        [&](std::int16_t v) { writer.WriteInt16(v); },
        [&](std::int32_t v) { writer.WriteInt32(v); },
        [&](std::int64_t v) { writer.WriteInt64(v); },
        [&](bool v) { writer.WriteBool(v); },
        [&](float v) { writer.WriteFloat(v); },
        [&](double v) { writer.WriteDouble(v); },
        [&](const Uuid& v) { writer.WriteUuid(v); },
        [&](const Timestamp& v) { writer.WriteTimestamp(v); },
        [&](const Date& v) { writer.WriteDate(v); },
        [&](const Time& v) { writer.WriteTime(v); },
        [&](const Blob& v) { writer.WriteBlob(v); },
        [&](const Inet& v) { writer.WriteInet(v); },
        [&](const std::shared_ptr<List>& v) { writer.WriteList(*v); },
        [&](const std::shared_ptr<Set>& v) { writer.WriteSet(*v); },
        [&](const std::shared_ptr<Map>& v) { writer.WriteMap(*v); }
    );
}

CassCollectionPtr BuildList(const List& list) {
    CassCollectionPtr col{cass_collection_new(CASS_COLLECTION_TYPE_LIST, list.items.size())};
    CollectionWriter writer{col.get()};
    for (const auto& item : list.items) {
        EncodeValue(writer, item);
    }
    return col;
}

CassCollectionPtr BuildSet(const Set& set) {
    CassCollectionPtr col{cass_collection_new(CASS_COLLECTION_TYPE_SET, set.items.size())};
    CollectionWriter writer{col.get()};
    for (const auto& item : set.items) {
        EncodeValue(writer, item);
    }
    return col;
}

CassCollectionPtr BuildMap(const Map& map) {
    CassCollectionPtr col{cass_collection_new(CASS_COLLECTION_TYPE_MAP, map.entries.size())};
    CollectionWriter writer{col.get()};
    for (const auto& [k, v] : map.entries) {
        EncodeValue(writer, k);
        EncodeValue(writer, v);
    }
    return col;
}

}  // namespace

void BindValue(CassStatement* statement, std::size_t index, const Value& value) {
    StatementWriter writer{statement, index};
    EncodeValue(writer, value);
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
