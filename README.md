# WebASIO

WebASIO is a modern C++20 service framework built on top of
[Boost.ASIO](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html).
It brings an ergonomic, programming model to asynchronous C++, letting you write
network and service code as straight-line coroutines instead of nested callbacks.

At its core is a lightweight coroutine layer (`webasio::coro`) that provides:

- **Lazy, awaitable promises** (`co_promise<T>`) — coroutines that start only
  when awaited and deliver their result, or propagate exceptions, at the
  `co_await` point.
- **Fire-and-forget tasks** (`co_detached`) for background work that manages
  its own lifetime.
- **Executor-aware scheduling** — `co_await`-able helpers such as `dispatch`,
  `post`, `sleep_for`, and access to the current `executor`, so work can hop
  between execution contexts explicitly.
- **First-class cancellation** integrated with ASIO's cancellation slots.
- **Seamless interop** with any Boost.ASIO async operation, awaited directly
  or with completion tokens like `as_tuple`.

## Requirements

- CMake 3.28 or newer
- A C++20-capable compiler (Clang recommended; the build uses the `-glldb` flag)
- [Ninja](https://ninja-build.org/) (recommended generator)
- Boost is vendored under `deps/boost`, so no system-wide install is required

## Building

The project uses an out-of-source build in the `build/` directory.

### Configure

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build. To pin the compiler,
pass `-DCMAKE_CXX_COMPILER=clang++`.

### Build

Build the library and example (unit tests are excluded from the default build):

```sh
cmake --build build
```

Build a single target:

```sh
cmake --build build --target webasio   # the library
cmake --build build --target example   # the example executable
```

## Running the example

```sh
./build/example/example
```

## Testing

The `unit_tests` target is excluded from the default build. Build and run the
tests together with the `check` target:

```sh
cmake --build build --target check
```

Alternatively, build the test binary and run CTest directly:

```sh
cmake --build build --target unit_tests
ctest --test-dir build --output-on-failure
```
