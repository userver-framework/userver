#ifdef USERVER_EHGLOBALS_INTERPOSE

// using custom section to escape LTO
__attribute__((section("cxa")))
// NOLINTNEXTLINE(hicpp-use-noexcept,modernize-use-noexcept)
extern "C" abi::__cxa_eh_globals*
__cxa_get_globals() throw() {
  return engine::impl::GetGlobals();
}

__attribute__((section("cxa")))
// NOLINTNEXTLINE(hicpp-use-noexcept,modernize-use-noexcept)
extern "C" abi::__cxa_eh_globals*
__cxa_get_globals_fast() throw() {
  return engine::impl::GetGlobals();
}

#endif
