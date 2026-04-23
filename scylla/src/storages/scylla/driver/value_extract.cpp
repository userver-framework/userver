#include "value_extract.hpp"

#include <array>
#include <chrono>
#include <cstring>

#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/scylla/operations.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

Value ExtractCollection(const CassValue* cass_val);
Value ExtractMap(const CassValue* cass_val);

Value ExtractScalar(const CassValue* cass_val) {
    if (!cass_val || cass_value_is_null(cass_val)) {
        return Value::Null();
    }

    switch (cass_value_type(cass_val)) {
        case CASS_VALUE_TYPE_VARCHAR:
        case CASS_VALUE_TYPE_TEXT:
        case CASS_VALUE_TYPE_ASCII: {
            const char* str = nullptr;
            std::size_t len = 0;
            cass_value_get_string(cass_val, &str, &len);
            return Value{std::string(str, len)};
        }
        case CASS_VALUE_TYPE_TINY_INT: {
            cass_int8_t v = 0;
            cass_value_get_int8(cass_val, &v);
            return Value{static_cast<std::int8_t>(v)};
        }
        case CASS_VALUE_TYPE_SMALL_INT: {
            cass_int16_t v = 0;
            cass_value_get_int16(cass_val, &v);
            return Value{static_cast<std::int16_t>(v)};
        }
        case CASS_VALUE_TYPE_INT: {
            cass_int32_t v = 0;
            cass_value_get_int32(cass_val, &v);
            return Value{static_cast<std::int32_t>(v)};
        }
        case CASS_VALUE_TYPE_BIGINT:
        case CASS_VALUE_TYPE_COUNTER: {
            cass_int64_t v = 0;
            cass_value_get_int64(cass_val, &v);
            return Value{static_cast<std::int64_t>(v)};
        }
        case CASS_VALUE_TYPE_BOOLEAN: {
            cass_bool_t v = cass_false;
            cass_value_get_bool(cass_val, &v);
            return Value{static_cast<bool>(v)};
        }
        case CASS_VALUE_TYPE_FLOAT: {
            cass_float_t v = 0.0f;
            cass_value_get_float(cass_val, &v);
            return Value{static_cast<float>(v)};
        }
        case CASS_VALUE_TYPE_DOUBLE: {
            cass_double_t v = 0.0;
            cass_value_get_double(cass_val, &v);
            return Value{static_cast<double>(v)};
        }
        case CASS_VALUE_TYPE_UUID:
        case CASS_VALUE_TYPE_TIMEUUID: {
            CassUuid u{};
            cass_value_get_uuid(cass_val, &u);
            return Value{Uuid{u.time_and_version, u.clock_seq_and_node}};
        }
        case CASS_VALUE_TYPE_TIMESTAMP: {
            cass_int64_t ms = 0;
            cass_value_get_int64(cass_val, &ms);
            return Value{Timestamp{std::chrono::milliseconds{ms}}};
        }
        case CASS_VALUE_TYPE_DATE: {
            cass_uint32_t d = 0;
            cass_value_get_uint32(cass_val, &d);
            return Value{Date{d}};
        }
        case CASS_VALUE_TYPE_TIME: {
            cass_int64_t ns = 0;
            cass_value_get_int64(cass_val, &ns);
            return Value{Time{ns}};
        }
        case CASS_VALUE_TYPE_BLOB: {
            const cass_byte_t* bytes = nullptr;
            std::size_t len = 0;
            cass_value_get_bytes(cass_val, &bytes, &len);
            Blob b(len);
            std::memcpy(b.data(), bytes, len);
            return Value{std::move(b)};
        }
        case CASS_VALUE_TYPE_INET: {
            CassInet inet{};
            cass_value_get_inet(cass_val, &inet);
            std::array<char, CASS_INET_STRING_LENGTH> buf{};
            cass_inet_string(inet, buf.data());
            return Value{Inet{std::string{buf.data()}}};
        }
        case CASS_VALUE_TYPE_LIST:
        case CASS_VALUE_TYPE_SET:
            return ExtractCollection(cass_val);
        case CASS_VALUE_TYPE_MAP:
            return ExtractMap(cass_val);
        default:
            return Value::Null();
    }
}

Value ExtractCollection(const CassValue* cass_val) {
    const auto kind = cass_value_type(cass_val);
    CassIteratorPtr it{cass_iterator_from_collection(cass_val)};
    std::vector<Value> items;
    while (cass_iterator_next(it.get())) {
        items.push_back(ExtractScalar(cass_iterator_get_value(it.get())));
    }
    if (kind == CASS_VALUE_TYPE_SET) {
        return Value{Set{std::move(items)}};
    }
    return Value{List{std::move(items)}};
}

Value ExtractMap(const CassValue* cass_val) {
    CassIteratorPtr it{cass_iterator_from_map(cass_val)};
    Map m;
    while (cass_iterator_next(it.get())) {
        m.entries.emplace_back(
            ExtractScalar(cass_iterator_get_map_key(it.get())),
            ExtractScalar(cass_iterator_get_map_value(it.get()))
        );
    }
    return Value{std::move(m)};
}

}  // namespace

Value ExtractValue(const CassValue* cass_val) { return ExtractScalar(cass_val); }

Row ExtractRow(const CassResult* result, const CassRow* cass_row) {
    Row::Cells cells;
    if (!cass_row) {
        return Row{std::move(cells)};
    }

    const std::size_t col_count = cass_result_column_count(result);
    cells.reserve(col_count);
    for (std::size_t i = 0; i < col_count; ++i) {
        const char* col_name = nullptr;
        std::size_t col_name_length = 0;
        cass_result_column_name(result, i, &col_name, &col_name_length);

        cells.emplace_back(std::string(col_name, col_name_length), ExtractScalar(cass_row_get_column(cass_row, i)));
    }
    return Row{std::move(cells)};
}

Rows ExtractAllRows(const CassResult* result) {
    Rows rows;
    rows.reserve(cass_result_row_count(result));

    CassIteratorPtr iterator{cass_iterator_from_result(result)};
    while (cass_iterator_next(iterator.get())) {
        rows.push_back(ExtractRow(result, cass_iterator_get_row(iterator.get())));
    }
    return rows;
}

operations::LwtResult ExtractLwtResult(const CassResult* result) {
    operations::LwtResult out;
    const CassRow* row = cass_result_first_row(result);
    if (!row) {
        return out;
    }

    Row::Cells previous_cells;
    const std::size_t col_count = cass_result_column_count(result);
    for (std::size_t i = 0; i < col_count; ++i) {
        const char* col_name = nullptr;
        std::size_t col_name_length = 0;
        cass_result_column_name(result, i, &col_name, &col_name_length);
        std::string name(col_name, col_name_length);

        const CassValue* cass_val = cass_row_get_column(row, i);
        if (name == "[applied]") {
            cass_bool_t applied = cass_false;
            if (cass_val && !cass_value_is_null(cass_val)) {
                cass_value_get_bool(cass_val, &applied);
            }
            out.applied = static_cast<bool>(applied);
            continue;
        }

        previous_cells.emplace_back(std::move(name), ExtractScalar(cass_val));
    }
    out.previous = Row{std::move(previous_cells)};
    return out;
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
