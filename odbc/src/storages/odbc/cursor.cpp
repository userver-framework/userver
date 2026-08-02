#include <userver/storages/odbc/cursor.hpp>

#include <utility>

#include <storages/odbc/detail/cursor_impl.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Cursor::Cursor(std::shared_ptr<detail::CursorImpl> impl)
    : impl_{std::move(impl)}
{}

Cursor::Cursor(Cursor&&) noexcept = default;
Cursor& Cursor::operator=(Cursor&&) noexcept = default;
Cursor::~Cursor() noexcept = default;

ResultSet Cursor::Fetch(std::size_t rows) {
    if (!impl_) {
        throw LogicError("ODBC cursor is moved-from");
    }
    return impl_->Fetch(rows);
}

bool Cursor::Done() const noexcept { return !impl_ || impl_->Done(); }

std::size_t Cursor::FetchedSoFar() const noexcept { return impl_ ? impl_->FetchedSoFar() : 0; }

}  // namespace storages::odbc

USERVER_NAMESPACE_END
