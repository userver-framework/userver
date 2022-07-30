Note to the future developers:

Everything related to AMQP-CPP resides in impl/, and AMQP-CPP objects are
supposed to live in dedicated ev-threads.

So we have a bunch of ev-threads, in which AMQP-CPP lives,
we have a bunch of coroutine threads, in which library objects live,
and we have some code in impl/ that basically calls `RunInEvLoop[Sync|Deferred]` 
to glue that together.

The only exception from this is the implementation of ConsumerBase,
which very carefully works with AMQP-CPP objects outside of impl/ directory,
and that should stay the only exception.
