# concurrencyts

## C++ Concurrency TS minimal implementation.

This is a bare-bones implementation of the [Concurrency TS](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4399.html). A simple if terse documentation is available [here](https://en.cppreference.com/w/cpp/experimental/concurrency).

This implementation provides the following abstractions.

1. `future` with `then continuations`, corresponding `promise` (`concurrencyts::future` in `future.h`).
2. An implementation of `async` that returns `future`s of the above kind (`concurrencyts::async` in `future.h`).
3. `when_all` for waiting on multiple futures till all are set (`concurrencyts::when_all` in `future.h`).
4. `when_any` akin to `when_all` but signaled as soon as any one future is ready.
5. `latch` for waiting till N threads have signaled the latch object (`concurrencyts::latch` in `latch.h`).
6. `barrier` and `flex_barrier` for upto N threads to block till all N have arrived (`concurrencyts::barrier` and `concurrencyts::flex_barrier` in `barrier.h`). Only `arrive_and_wait` is implemented, while `arrive_and_drop` is missing.

The rest of the abstractions are absent. While I intend to add them, I don't have a fixed schedule for doing so.

## Compatibility
You can use this library with a compiler that supports C++14 or later. It will possibly work with C++11 with minor modifications (change the `std::result_of_t<...>`s to `std::result_of<...>::type`. We don't use any mostrosity beyond simple GNUMake for building. It has been tested with g++/libstdc++ as well as clang++/libc++ on Linux. It works unchanged with C++17 and C++20.

## Building
The library is entirely header-only, and doesn't use any virtual functions, etc. Thus in order to use the future-related functionality, you just include the header `future.h`, and get all symbols from the namespace `concurrencyts`. Likewise with the other headers.

If you would rather use the `std::experimental` namespace, just `#include "concurrencyts.h"`. Don't include any other header individually.

You can check example usages of the abstractions by looking at the corresponding `.cc` files. You can also build and run them using:

    make bin/future
    make bin/latch
    make bin/barrier

There are no formal unit tests, but I intend to add them soon.
