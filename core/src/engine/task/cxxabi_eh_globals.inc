#ifdef USERVER_EHGLOBALS_INTERPOSE

extern "C" {

// using custom section to escape LTO
__attribute__((section("cxa"))) abi::__cxa_eh_globals*
// NOLINTNEXTLINE(hicpp-use-noexcept,modernize-use-noexcept)
__cxa_get_globals() throw() {
  return engine::impl::GetGlobals();
}

__attribute__((section("cxa"))) abi::__cxa_eh_globals*
// NOLINTNEXTLINE(hicpp-use-noexcept,modernize-use-noexcept)
__cxa_get_globals_fast() throw() {
  return engine::impl::GetGlobals();
}

}  // extern "C"

#endif
