#pragma once

#ifdef __has_attribute

#if __has_attribute(__nodebug__)
#define USERVER_IMPL_NODEBUG __attribute__((__nodebug__))
#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__nodebug__, __always_inline__))
#elif __has_attribute(__always_inline__)
// GCC may have no __nodebug__ attribute, but __always_inline__ behaves quite closely - it inlines the function
// without providing an external definition
#define USERVER_IMPL_NODEBUG_INLINE_FUNC __attribute__((__always_inline__))
#endif

#endif

#ifndef USERVER_IMPL_NODEBUG
#define USERVER_IMPL_NODEBUG
#endif

#ifndef USERVER_IMPL_NODEBUG_INLINE_FUNC
#define USERVER_IMPL_NODEBUG_INLINE_FUNC
#endif
