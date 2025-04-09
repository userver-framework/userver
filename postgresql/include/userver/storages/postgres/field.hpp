#pragma once

/// @file
/// @brief Result field accessors

#include <cstddef>
#include <type_traits>
#include <utility>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/supported_types.hpp>

#include <userver/storages/postgres/detail/const_data_iterator.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct FieldDescription {
    /// Index of the field in the result set
    std::size_t index;
    /// @brief The object ID of the field's data type.
    Oid type_oid;
    /// @brief The field name.
    // TODO string_view
    std::string name;
    /// @brief If the field can be identified as a column of a specific table,
    /// the object ID of the table; otherwise zero.
    Oid table_oid;
    /// @brief If the field can be identified as a column of a specific table,
    /// the attribute number of the column; otherwise zero.
    Integer table_column;
    /// @brief The data type size (see pg_type.typlen). Note that negative
    /// values denote variable-width types.
    Integer type_size;
    /// @brief The type modifier (see pg_attribute.atttypmod). The meaning of
    /// the modifier is type-specific.
    Integer type_modifier;
};

class FieldView final {
public:
    using size_type = std::size_t;

    FieldView(const detail::ResultWrapper& res, size_type row_index, size_type field_index)
        : res_{res}, row_index_{row_index}, field_index_{field_index} {}

    template <typename T>
    size_type To(T&& val) const {
        using ValueType = typename std::decay<T>::type;
        auto fb = GetBuffer();
        return ReadNullable(fb, std::forward<T>(val), io::traits::IsNullable<ValueType>{});
    }

private:
    io::FieldBuffer GetBuffer() const;
    std::string_view Name() const;
    Oid GetTypeOid() const;
    const io::TypeBufferCategory& GetTypeBufferCategories() const;

    template <typename T>
    size_type ReadNullable(const io::FieldBuffer& fb, T&& val, std::true_type) const {
        using ValueType = typename std::decay<T>::type;
        using NullSetter = io::traits::GetSetNull<ValueType>;
        if (fb.is_null) {
            NullSetter::SetNull(val);
        } else {
            Read(fb, std::forward<T>(val));
        }
        return fb.length;
    }

    template <typename T>
    size_type ReadNullable(const io::FieldBuffer& buffer, T&& val, std::false_type) const {
        if (buffer.is_null) {
            throw FieldValueIsNull{field_index_, Name(), val};
        } else {
            Read(buffer, std::forward<T>(val));
        }
        return buffer.length;
    }

    template <typename T>
    void Read(const io::FieldBuffer& buffer, T&& val) const {
        using ValueType = typename std::decay<T>::type;
        io::traits::CheckParser<ValueType>();
        try {
            io::ReadBuffer(buffer, std::forward<T>(val), GetTypeBufferCategories());
        } catch (InvalidInputBufferSize& ex) {
            // InvalidInputBufferSize is not descriptive. Enriching with OID information and C++ types info
            ex.AddMsgPrefix(fmt::format(
                "Error while reading field #{0} '{1}' which database type {2} as a C++ type '{3}'. Refer to "
                "the 'Supported data types' in the documentation to make sure that the database type is actually "
                "representable as a C++ type '{3}'. Error details: ",
                field_index_,
                Name(),
                impl::OidPrettyPrint(GetTypeOid()),
                compiler::GetTypeName<T>()
            ));
            UASSERT_MSG(false, ex.what());
            throw;
        } catch (ResultSetError& ex) {
            ex.AddMsgSuffix(fmt::format(" (ResultSet error while reading field #{} name `{}`)", field_index_, Name()));
            throw;
        }
    }

    const detail::ResultWrapper& res_;
    const size_type row_index_;
    const size_type field_index_;
};

/// @brief Accessor to a single field in a result set's row
class Field {
public:
    using size_type = std::size_t;

    size_type RowIndex() const { return row_index_; }
    size_type FieldIndex() const { return field_index_; }

    //@{
    /** @name Field metadata */
    /// Field name as named in query
    std::string_view Name() const;
    FieldDescription Description() const;

    Oid GetTypeOid() const;
    //@}

    //@{
    /** @name Data access */
    bool IsNull() const;

    size_type Length() const;

    /// Read the field's buffer into user-provided variable.
    /// @throws FieldValueIsNull If the field is null and the C++ type is
    ///                           not nullable.
    template <typename T>
    size_type To(T&& val) const {
        return FieldView{*res_, row_index_, field_index_}.To(std::forward<T>(val));
    }

    /// Read the field's buffer into user-provided variable.
    /// If the field is null, set the variable to the default value.
    template <typename T>
    void Coalesce(T& val, const T& default_val) const {
        if (!IsNull())
            To(val);
        else
            val = default_val;
    }

    /// Convert the field's buffer into a C++ type.
    /// @throws FieldValueIsNull If the field is null and the C++ type is
    ///                           not nullable.
    template <typename T>
    typename std::decay<T>::type As() const {
        T val{};
        To(val);
        return val;
    }

    /// Convert the field's buffer into a C++ type.
    /// If the field is null, return default value.
    template <typename T>
    typename std::decay<T>::type Coalesce(const T& default_val) const {
        if (IsNull()) return default_val;
        return As<T>();
    }
    //@}
    const io::TypeBufferCategory& GetTypeBufferCategories() const;

protected:
    friend class Row;

    Field() = default;

    Field(detail::ResultWrapperPtr res, size_type row, size_type col)
        : res_{std::move(res)}, row_index_{row}, field_index_{col} {}

    //@{
    /** @name Iteration support */
    bool IsValid() const;
    int Compare(const Field& rhs) const;
    std::ptrdiff_t Distance(const Field& rhs) const;
    Field& Advance(std::ptrdiff_t);
    //@}

private:
    detail::ResultWrapperPtr res_;
    size_type row_index_{0};
    size_type field_index_{0};
};

/// @brief Iterator over fields in a result set's row
class ConstFieldIterator
    : public detail::ConstDataIterator<ConstFieldIterator, Field, detail::IteratorDirection::kForward> {
public:
    ConstFieldIterator() = default;

private:
    friend class Row;

    ConstFieldIterator(detail::ResultWrapperPtr res, size_type row, size_type col)
        : ConstDataIterator(std::move(res), row, col) {}
};

/// @brief Reverse iterator over fields in a result set's row
class ReverseConstFieldIterator
    : public detail::ConstDataIterator<ReverseConstFieldIterator, Field, detail::IteratorDirection::kReverse> {
public:
    ReverseConstFieldIterator() = default;

private:
    friend class Row;

    ReverseConstFieldIterator(detail::ResultWrapperPtr res, size_type row, size_type col)
        : ConstDataIterator(std::move(res), row, col) {}
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
