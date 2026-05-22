# Tuning Guide

How to run this engine on a production HFT box: what to tune, why
each knob matters, and what to trade off. The guide covers concepts
and config patterns; rollout scripts live with the deployment
repository. Numbers in snippets are starting points, not
prescriptions -- size them to the host and the link.

## Why this engine needs tuning

The pipeline runs three threads, each driving a `kraken::event_loop`
with `kraken::busy_spin_idle` (see [`../README.md`](../README.md)). A
busy-spin consumer never sleeps: every cycle the kernel steals --
timer ticks, RCU callbacks, NIC interrupts, scheduler decisions,
power-state transitions -- shows up directly as added latency on the
order path. System tuning aims to leave those three cores alone so
the only thing they run is the engine's loops.

Three thread roles map cleanly to three isolated cores:

| Loop | Hot work | Tuning emphasis |
|------|----------|-----------------|
| Input | UDP receive, CSV decode, route | NIC, IRQ steering, kernel-bypass |
| Processing | Order books, matching | Cache locality, no preemption |
| Output | CSV encode, stdout writeback | No preemption; tolerates more jitter than input or processing |

Everything below either takes work off these cores, or shortens the
path packets and memory take to reach them.

## Layers of tuning

From the lowest layer up:

1. Kernel boot parameters (`/etc/default/grub`)
2. CPU frequency and idle behaviour (sysfs)
3. Memory: huge pages, THP, NUMA balancing, KSM
4. Network sysctls
5. NIC settings (`ethtool`)
6. Kernel-bypass networking (Solarflare)
7. Process placement (cpuset, NUMA, threading)
8. Verification

Each layer compounds on the ones below it. Start from the bottom;
upper layers assume the lower ones are already in place.

## 1. Kernel boot parameters

The kernel command line is the only place to hand entire cores to a
single workload and to disable kernel features that no amount of
runtime tuning can undo. Edit `GRUB_CMDLINE_LINUX_DEFAULT` in
`/etc/default/grub`, run `update-grub`, and reboot.

```text
isolcpus=2-19 nohz=on nohz_full=2-19 skew_tick=1 \
rcu_nocbs=2-19 rcu_nocb_poll \
idle=poll \
transparent_hugepage=never \
selinux=0 audit=0 mitigations=off
```

- **CPU isolation and tickless operation**
  - `isolcpus=<range>` removes the listed cores from the general
    scheduler; nothing migrates onto them unless explicitly pinned.
  - `nohz_full=<range>` stops the periodic 1 kHz timer tick on those
    cores when only one task is runnable -- the dominant source of
    sub-microsecond jitter on a quiet core.
  - `skew_tick=1` staggers ticks across cores so the housekeeping
    cores do not all run their tick code at the same instant.

- **RCU offload**
  - `rcu_nocbs=<range>` moves RCU callback processing off the
    isolated cores onto dedicated `rcuog` kthreads on other cores.
  - `rcu_nocb_poll` makes those kthreads poll instead of being IPI'd,
    which removes the wake-up interrupt from the isolated cores.

- **Idle behaviour**
  - `idle=poll` forbids any C-state on idle. The CPU busy-waits.
    This is the minimum-latency setting; it raises power draw and
    heat to roughly full-load levels even while idle. Cooling must
    be sized for it.

- **Memory**
  - `transparent_hugepage=never` disables THP. THP background
    coalescing scans pages and migrates allocations, producing
    unpredictable multi-millisecond stalls. The runtime fallback,
    when changing the boot line is not an option, is to write
    `never` into `/sys/kernel/mm/transparent_hugepage/enabled`.
    Reserve explicit huge pages instead (see section 3).

- **Security and audit**
  - `selinux=0`, `audit=0`, `mitigations=off` trade kernel safety
    nets for cycles. `mitigations=off` alone can recover a meaningful
    fraction of syscall and context-switch cost on affected silicon.
    Only acceptable on a host whose threat model already isolates it
    from untrusted workloads.

Cross-check after reboot:

```bash
cat /proc/cmdline
cat /sys/devices/system/cpu/isolated
cat /sys/devices/system/cpu/nohz_full
```

## 2. CPU frequency and idle

Even with `idle=poll`, the frequency governor decides what clock the
isolated cores run at. The default is `powersave` or `schedutil`,
which down-clock the core whenever it dips below a threshold and pay
the wake-up cost on the way back up.

```bash
for governor in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
  echo performance | sudo tee "$governor"
done
```

For deeper control, disable Turbo Boost and set the minimum frequency
equal to the maximum: a constant clock removes another source of
variance. Configure this in BIOS where possible (it survives
reboots) and as a fallback through `cpupower frequency-set` or the
`intel_pstate` sysfs knobs.

The trade-off mirrors `idle=poll`: power, heat, and electricity bill
in exchange for predictability.

## 3. Memory

The aim is a steady-state heap with no background page management
visible from the hot path.

- **Explicit huge pages.** Pre-allocate 2 MiB pages at boot and let
  the allocator (or the application) draw from them. TLB pressure
  drops sharply; latency stops depending on whether the working set
  happens to span a recently re-mapped page.

  On a single-socket host, set the global total:

  ```bash
  sysctl -w vm.nr_hugepages=4096
  ```

  On a multi-socket host, allocate per NUMA node so the engine's
  pages land on the same socket as its cores:

  ```bash
  echo 4096 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
  echo 4096 > /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages
  ```

- **NUMA balancing off.** Automatic NUMA balancing periodically
  unmaps pages and lets the next access fault them back in on the
  accessing node. Useful for general workloads, fatal for a busy-spin
  loop that touches the same pages every microsecond.

  ```bash
  sysctl -w kernel.numa_balancing=0
  ```

- **Kernel Samepage Merging off.** KSM scans memory looking for
  identical pages to deduplicate. It saves RAM on VM hosts and
  introduces unpredictable scan-induced stalls everywhere else.

  ```bash
  echo 0 > /sys/kernel/mm/ksm/run
  ```

Pin the engine to a single NUMA node when possible: receive on the
NIC's local node, run the matching loop on the same node, write
stdout on a non-isolated housekeeping core on either node. Memory
allocated on one node and accessed from another adds tens of
nanoseconds per cache line.

## 4. Network sysctls

The kernel networking stack ships with conservative socket-buffer
limits sized for general-purpose servers. A market-data receiver
needs much larger limits to absorb burst microstructure without
drops between the NIC and the userspace ring.

```bash
sysctl -w net.core.rmem_max=536870912        # 512 MiB
sysctl -w net.core.wmem_max=268435456        # 256 MiB
sysctl -w net.core.netdev_max_backlog=10000  # per-CPU input queue

# TCP defaults if any TCP session shares the host (drop-copy, control)
sysctl -w net.ipv4.tcp_rmem="4096 87380 268435456"
sysctl -w net.ipv4.tcp_wmem="4096 16777216 268435456"
```

`rmem_max` bounds what `setsockopt(SO_RCVBUF)` can request; the
application must still ask for it explicitly. The engine's
`asio_udp_receiver` keeps its socket options minimal -- raise the
`SO_RCVBUF` ceiling here at the OS level and add an explicit
`socket.set_option(boost::asio::socket_base::receive_buffer_size{...})`
on the receiver when deployment-side burst absorption matters.

Disable reverse-path filtering on the market-data interface. With
multi-homed hosts and asymmetric routing -- the common case on a
trading rack -- `rp_filter` silently drops legitimate inbound packets
because the reply route does not match.

```bash
echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter
echo 0 > /proc/sys/net/ipv4/conf/<interface>/rp_filter
```

## 5. NIC settings (`ethtool`)

The NIC contributes to latency in three places: interrupt coalescing,
checksum/offload paths, and ring sizing. Tune each per interface
carrying market data.

```bash
# zero coalescing: deliver every frame as it arrives, no batching
ethtool -C <interface> rx-usecs 0 tx-usecs 0 \
                      rx-usecs-irq 0 tx-usecs-irq 0 \
                      adaptive-rx off

# enable RX/TX checksum offload and TSO; the NIC does the work, not the CPU
ethtool -K <interface> rx on tx on tso on
ethtool --offload <interface> rx on tx on

# enlarge rings to ride out micro-bursts without dropping
ethtool -G <interface> rx 4096 tx 2048
```

The interrupt-coalescing change is the one that matters most. Zero
coalescing trades NIC efficiency (more interrupts, more PCIe
transactions) for lower per-packet latency.

For a pure kernel-bypass deployment (section 6), most `ethtool`
settings become irrelevant because the bypass stack drives the NIC
directly. The two that still matter are `rp_filter` (section 4) and
ring sizing, since both stacks share the underlying ring memory.

Pin NIC interrupts off the isolated cores. By default `irqbalance`
spreads them across all online CPUs, which routinely lands them on
the cores running the engine.

```bash
systemctl stop irqbalance
# write a CPU mask listing only housekeeping cores
echo <mask> > /proc/irq/<irq_number>/smp_affinity
```

## 6. Kernel-bypass networking (Solarflare)

A standard kernel stack still spends microseconds per packet on
syscall transitions, sk_buff allocation, and softirq dispatch. For
sub-microsecond receive paths, bypass the kernel and let userspace
talk to the NIC directly.

Onload, TCPDirect, and ef_vi are licensed Solarflare/AMD products.
The engine builds and runs without them; kernel-bypass paths remain
opt-in behind config, not the default.

Solarflare ships three layered options that share the same NIC and
driver and differ in API surface, integration effort, and latency
floor:

| Option | API | Integration effort | Latency win | Pick when |
|--------|-----|--------------------|-------------|-----------|
| OpenOnload | BSD sockets via `LD_PRELOAD` | None -- launcher only | Good | Existing socket-based code; mixed TCP/UDP; opt in per deploy |
| TCPDirect | libzf (`zf_*` poll-driven API) | New API surface | Better for TCP | TCP order path; willing to integrate against a vendor API |
| ef_vi | Raw frame access | High -- parse Ethernet/IP yourself | Best | Fixed-protocol UDP, typically market-data multicast |

The three are not mutually exclusive. A typical deployment runs ef_vi
for inbound multicast market data and Onload (or TCPDirect) for the
outbound TCP order path on the same NIC.

### OpenOnload

OpenOnload intercepts the BSD socket API with `LD_PRELOAD` and
serves accelerated sockets out of a userspace stack. The application
stays portable -- it still calls `socket()`, `recvfrom()`, `send()`
-- and Onload routes traffic on accelerated interfaces through the
kernel-bypass path. The change is the launcher and a profile file:

```bash
onload --profile=/path/to/latency.opf ./kraken_submission ./config.json
```

The profile is a list of `onload_set` directives that tune the
Onload stack. A typical low-latency profile mixes spin-tuning,
buffer sizing, and feature toggles:

```text
# latency.opf
onload_set EF_PIO 1                 # programmed I/O for tiny TX packets
onload_set EF_FORCE_TCP_NODELAY 1   # disable Nagle on every socket
onload_set EF_STACK_PER_THREAD 0    # one stack shared across threads
onload_set EF_TCP_FASTSTART_INIT 0  # no slow-start ramp on new connections
onload_set EF_TCP_FASTSTART_IDLE 0
onload_set EF_TCP_CLIENT_LOOPBACK 4 # short-circuit loopback sockets
onload_set EF_TCP_SERVER_LOOPBACK 2

# NIC ring sizes -- match the hardware ceiling
onload_set EF_RXQ_SIZE 4096
onload_set EF_TXQ_SIZE 2048

# packet pool -- bias toward TX which is more burst-prone
onload_set EF_MAX_PACKETS    131072
onload_set EF_MAX_TX_PACKETS  98304
onload_set EF_MAX_RX_PACKETS  32768

# socket buffer overrides
onload_set EF_TCP_RCVBUF_MODE 0
onload_set EF_TCP_RCVBUF      67108864
onload_set EF_TCP_SNDBUF_MODE 1
onload_set EF_TCP_SNDBUF      67108864
```

The full `EF_*` reference is in the Onload user guide; the values
above are starting points for a market-data receiver on a 10/25 GbE
Solarflare NIC. Pin the Onload stack to the NIC's NUMA node;
otherwise it allocates ring buffers on the wrong socket. The
mechanism is `onload_tool reload` after `numactl`-ing the cores.

### TCPDirect

TCPDirect drops the BSD socket abstraction entirely and exposes a
poll-driven API (`zf_reactor_perform`) over Solarflare's libzf.
Latency is lower than Onload: the API is closer to the NIC, and the
application controls polling cadence. The cost is a different API
surface; the application either uses TCPDirect natively or wires in
through a vendor library that does.

The OnixS FIX engine, for example, takes a stack object built from a
small attribute table. The same fields surface as configuration on
the runtime side:

```json
{
  "network_interface": "ens1f0",
  "tcpdirect": {
    "tcp_initial_cwnd": 10,
    "tx_ring_max": 2048,
    "ctpio": 2,
    "ctpio_mode": "ct",
    "reactor_spin_count": 10000
  }
}
```

What each setting does:

- `tcp_initial_cwnd` -- initial congestion window per new zocket;
  larger values let the first burst go out without waiting for ACKs.
- `tx_ring_max` -- TX descriptor ring size; bigger rings absorb
  bursts at the cost of memory.
- `ctpio` and `ctpio_mode` -- cut-through PIO. Mode `ct` starts
  transmitting the frame before the application has finished writing
  it, shaving roughly 100 ns off the send path. Mode `sf` is the
  safer store-and-forward variant; `sf-np` guarantees no poisoned
  frames at the wire.
- `reactor_spin_count` -- how many spins the reactor takes through
  the event loop with no work before returning. Higher means lower
  latency when work appears (the loop is already hot) at the price
  of more CPU consumed when idle.

### ef_vi

ef_vi is the lowest layer: a virtual NIC interface that delivers raw
Ethernet frames to userspace and accepts raw frames back. There is
no stack at all; the application parses headers itself. Use it when
the messaging layer is a fixed binary protocol (typically UDP
multicast market data) and the saving over Onload matters more than
the simplicity of `recvfrom()`.

This submission carries a stub `kraken::network::ef_vi_udp_receiver`
behind the same callback shape as the `asio_udp_receiver`, so the
wiring shell can swap transports without touching the decoder. A
real implementation allocates a virtual interface, registers an
RX filter for the multicast group, posts receive descriptors, and
parses Ethernet/IP/UDP off raw frames each `poll()` call.

## 7. Process placement

Boot-time isolation provides cores; runtime placement puts threads
on them. Pin each engine thread to a specific isolated core, and
keep housekeeping threads (storage, logging, telemetry) off those
cores.

- **Thread pinning.** Pin each engine thread to a dedicated isolated
  core with `taskset -c <core>` on the launcher, or with `cset shield`
  on a deployment that manages a shielded cpuset. The three loops
  carry stable thread names (`kraken-input`, `kraken-engine`,
  `kraken-output`) so per-thread affinity can also be applied from
  outside the process via `/proc/<pid>/task/<tid>` once the loops
  are up.
- **NUMA placement.** `numactl --cpunodebind=0 --membind=0` keeps
  allocations on the same node as the cores and the NIC. Mismatched
  binding wastes most of what the earlier layers buy.
- **`ulimit -c unlimited`** for production: a core file from a once-
  in-a-quarter crash is worth far more than the memory it took.
- **Real-time priority.** `chrt -f 50 ./kraken_submission ...` runs
  the process under SCHED_FIFO. With cores isolated and ticks off
  there is rarely anything else runnable, but the FIFO priority
  guarantees the busy-spin loop wins if anything ever migrates onto
  the core.

## 8. Verification

A tuning change that is not measured is a guess. After each layer
goes in, confirm both that it took effect and that it moved the
number that matters.

- **Took effect.** `/proc/cmdline`,
  `cat /sys/devices/system/cpu/isolated`,
  `cat /sys/devices/system/cpu/cpu<N>/cpufreq/scaling_governor`,
  `ethtool -c <interface>`, `onload_stackdump lots`.
- **Moved the number.** Run the engine's microbenchmarks (see
  `../submission/benchmarks/order_book/`) before and after each
  layer. Track p50 and p99 separately; the tuning that helps p99 the
  most (interrupt steering, idle, NUMA) is often invisible at p50.
- **Find new jitter.** `perf sched record`, `bcc/runqlat`,
  `bcc/hardirqs`, and `cyclictest` all surface where time still
  leaves the busy-spin loop after the obvious sources are gone.

## See also

- Application-level tuning -- allocator choice, container sizing,
  branch hints -- in [`cpp-design-principles.md`](cpp-design-principles.md).
- Benchmark methodology in `../submission/benchmarks/order_book/`.
- BIOS and firmware (HT disable, P-state ownership, PCIe lane
  layout) live with the hardware runbook.
