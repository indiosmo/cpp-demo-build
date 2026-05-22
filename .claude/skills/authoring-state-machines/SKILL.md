---
name: authoring-state-machines
description: Design, implement, and test Boost.SML state machines in matching-engine-lab.
---

# Authoring State Machines

Use this skill when a lifecycle is complex enough to deserve an explicit
Boost.SML state machine. The generic mechanics live in
[`docs/cpp-guidelines/cpp-design-principles/state-machines.md`](../../../docs/cpp-guidelines/cpp-design-principles/state-machines.md).
The lab-specific mapping lives in
[`docs/lab-guidelines/design.md`](../../../docs/lab-guidelines/design.md).

This repo is being reframed as a C++26 portfolio demonstrator. A state machine
should make a lifecycle easier to inspect and discuss; it should not add
ceremony to simple matching-engine branches.

## Required Reading

1. Read
   [`state-machines.md`](../../../docs/cpp-guidelines/cpp-design-principles/state-machines.md).
   Start with "When a state machine is the right tool" and "When not to use
   one". If the use case does not fit, stop and propose `std::variant` +
   `lab::match`, an `enum class` switch, or a plain function.
2. Read
   [`docs/lab-guidelines/design.md`](../../../docs/lab-guidelines/design.md),
   especially the C++26 target, utility layer, runtime split, and callback
   wiring sections.
3. If tests are non-trivial, also use the
   [`authoring-tests`](../authoring-tests/SKILL.md) skill.

## Expected Layout

State machines live with the owning module:

```text
src/<module>/<module>/detail/<name>_state_machine.hpp
test/<module>/src/test_<name>_state_machine.cpp
```

During the reframe, current implementation files may still sit under
`submission/src/` and `submission/test/`. Preserve buildability while moving
toward the target root layout.

## Workflow

### 1. Design

Draw the lifecycle in PlantUML in a block comment above the transition table.
The diagram is the readable spec; the table mirrors it. When the transition
table changes, update the diagram in the same change.

Use matching-engine vocabulary in state and event names. Prefer concrete names
such as `ev_order_accepted`, `ev_fill_complete`, `st_resting`, or
`st_draining` over generic names such as `ev_success` or `st_done`.

### 2. Implement

Namespace convention:

- events, states, actions, and transitions live in
  `<module>::detail::<name>_state_machine_fsm`;
- the public template alias lives in `<module>::detail`.

Project conventions:

- Prefix events with `ev_` and states with `st_`.
- Wrap empty event/state struct blocks in `// clang-format off` and
  `// clang-format on` when needed to keep them one per line.
- Store side effects in an `actions` aggregate using `lab::inplace_function`
  callback fields.
- Keep the FSM as sequencing only. Business logic belongs in the owner, called
  through private `fsm_*` methods.
- Provide a template alias over policies so tests can inject
  `boost::sml::testing`:

```cpp
template <typename... Policies>
using <name>_state_machine =
  boost::sml::sm<<name>_state_machine_fsm::transitions, Policies...>;
```

Fully qualify `boost::sml::on_entry<_>` and `boost::sml::on_exit<_>` at the
table site to avoid ambiguous lookup.

### 3. Integrate

Member declaration order matters when the FSM holds references to actions:

```cpp
private:
  detail::<name>_state_machine_fsm::actions fsm_actions_{
    .publish_ack{[this] { fsm_publish_ack(); }},
    .publish_reject{[this] { fsm_publish_reject(); }},
  };

  detail::<name>_state_machine<> fsm_{fsm_actions_};
```

Drive the FSM with `fsm_.process_event(fsm::ev_xxx{})`. Expose semantic
accessors such as `bool resting() const`; do not leak
`fsm_.is(boost::sml::state<X>)` to callers.

### 4. Test

Use `boost::sml::testing` in unit tests:

```cpp
<name>_state_machine<boost::sml::testing> machine{actions};
```

Clear the action log after every `set_current_states()`, because entry actions
can fire during the jump.

Test shape:

- one isolated test per transition edge;
- one full lifecycle happy path;
- dedicated tests for retry, error, and recovery loops;
- owner integration tests only for wiring between the owner and the FSM.

## Done Criteria

- PlantUML and transition table match edge-for-edge, including guards and
  actions.
- Every transition edge has an isolated test.
- At least one full-lifecycle test walks the happy path.
- Every retry, error, or recovery loop has a dedicated test.
- The owner exposes semantic accessors, not raw SML state checks.
- Every action callback is wired in production and tests.
- The design still supports the portfolio goal: easier to inspect, explain,
  and extend than an equivalent ad hoc branch chain.
