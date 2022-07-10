Underlying library (AMQP-CPP) is not thread-safe 
and every object of that library is supposed to live in an event loop.

You shouldn't call any methods on those objects outside of their dedicated ev-thread, 
even if them methods are just getters: again, there is NO synchronization.

AMQP-CPP is callbacks-based, so you should be extra careful when capturing in lambdas:
1. Make sure your capture doesn't go out of scope too early (shared_ptr helps here)
2. Make sure you don't accidentally leak: for how long is your shared_ptr going to be stored 
in a callback? Does it get reset if a callback never fires?
3. Make sure callbacks are reset: say channel.onError might fire long after channel was created,
and since channels are pooled it might even fire when another object uses it
(this is taken care of, but you get the idea: capturing `this` is dangerous)
