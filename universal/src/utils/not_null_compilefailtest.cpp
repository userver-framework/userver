#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

void f() {
    // FAILNEXTLINE(NotNull does not work with references)
    utils::NotNull<int&> ref;

    // FAILNEXTLINE(NotNull does not work with const T)
    utils::NotNull<const int> const_int;
}

void g() {
    // FAILNEXTLINE(NotNull does not work with references)
    utils::NotNull<int&> ref;
}

USERVER_NAMESPACE_END
