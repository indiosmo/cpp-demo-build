# lab::network

UDP ingress sub-namespace. Two concrete receiver backends live here, and
the wiring shell binds to whichever it composes; the rest of the
codebase depends on `lab::network::` symbols rather than on Boost.Asio
or any vendor SDK directly.

## Backends

- **`asio_udp_receiver`** -- Boost.Asio-backed receiver. The
  `io_context` is injected so the wiring shell can mux it onto the same
  event-loop thread that drives other Asio work; polling the context
  is the owner's job. Delivery is callback-driven: `on_datagram` fires
  for each successful receive.

- **`ef_vi_udp_receiver`** -- Solarflare kernel-bypass receiver. Stub
  in this tree, kept compilable so the CMake split and include topology
  can be exercised without the vendor libraries on the host. Native
  model is poll-driven: the caller drives a busy loop and `poll()`
  drains the receive event queue per tick.

The two backends do not share an abstract base. Their control-flow
models diverge sharply -- callback-driven event loop versus
caller-polled bursts -- so squeezing both into a single virtual API
would either fake an event loop on top of a busy poll or push Asio's
callback model onto a thread that just wants to spin. The wiring shell
selects a backend at composition time instead.

The shared payload contract (`endpoint_config`, `datagram_view`) lives
in `lab/network/types.hpp`; see the receiver headers for the
backend-specific lifecycle.

## CMake split

Each backend is its own static library:

- `lab::network::asio` -- pulls in Boost.Asio header-only and confines
  the `BOOST_ASIO_HEADER_ONLY` define plus the localized
  `-Wno-null-dereference` suppression to this target. Consumers see the
  receiver through its concrete API without inheriting Boost transitively.
- `lab::network::ef_vi` -- stand-in for the Solarflare SDK target. A
  real integration would gate `find_package(EfVi REQUIRED)` and the
  Solarflare imported targets behind an opt-in flag so the asio path
  stays vendor-free when the flag is off.

## Examples

`examples/` ships a standalone program (`asio_udp_receiver_example`)
that exercises the receiver lifecycle end-to-end -- construct with an
injected `io_context`, wire `on_datagram`, `start()`, drive the
context, `stop()` -- without any of the project's pipeline plumbing
around it. Useful for seeing how to drop the receiver into a different
runtime, or for sanity-checking the socket round-trip with `nc`.

Examples build when the project is configured with
`-DLAB_BUILD_EXAMPLES=ON`.
