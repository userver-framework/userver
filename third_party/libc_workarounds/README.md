This library contains workarounds for libc when used in taxi/uservices.

It corrects libc behavior in regards to thread-switching execution contexts.
Namely, moving execution contexts between threads breaks errno and pthread_self
accesses with optimizations. This happens because their implementations
are marked as not dependend on an external state of the program.

These patches were deemed not acceptable in musl upstream (works as designed)
and should not be used anywhere else.

Discussion in musl mailing list: https://www.openwall.com/lists/musl/2021/04/08/
Task for removal: https://st.yandex-team.ru/TAXICOMMON-3815
