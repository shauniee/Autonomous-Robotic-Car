# Project C coding standard

Apply these rules to new or changed application code; the [minimum-bit rules](#minimum-bit-data) apply to all application data. They are a concise project adoption informed by Barr Group's 2018 embedded C guidance; they do not certify complete compliance or require reformatting the imported kernel.

## Language and layout

- The build uses **GNU C11** (`-std=gnu11`). Use C11 acquire/release loads and stores for word snapshots and startup publication; atomic read-modify-write helpers are not available on this ARMv6-M (Cortex-M0+) target. Isolate other necessary compiler/hardware extensions in drivers and document them.
- Use four spaces, LF endings, one statement per line and braces for every control body. Put opening braces on their own line; aim for 80-character lines.
- Space binary operators, clarify precedence with parentheses, and comment purpose, units, bounds and assumptions. Remove unused/commented-out code.

## Modules and data

- Give each module one purpose and matching lowercase `snake_case.c` / `.h` names. Include its own guarded header; expose only needed declarations. Never include a `.c` file or absolute include path.
- Prefix public functions with the module name; make private functions/data `static`. Use descriptive names and named constants. Prefer functions to macros; required macros evaluate arguments once and parenthesize expressions.
- Use fixed-width integers where width matters, packed bits for stored or shared flags, `bool` only for locals and return values, and `const` for immutable data. Keep unsigned literals/ranges consistent; justify narrowing casts and avoid signed bit operations.
- Initialize data before use; represent unavailable values explicitly with a documented invalid code. New types may use module-prefixed `snake_case_t`.
- Use `float` only for local computation where justified, with `f` suffixes and finite/range checks. Stored and shared values use fixed point with a documented scale unless [DATA_REPRESENTATION](DATA_REPRESENTATION.md) justifies a float.
- C bit-fields may hold in-RAM packed state. Wire formats (MQTT) use explicit masks and shifts with documented byte order; never send native structs or rely on padding or bit-field order.

## Control and concurrency

- Keep functions reviewable; check return codes and choose safe failure responses. Include `default` in switches and document intentional fallthrough. Early error returns are acceptable.
- Bound hardware polling, waits, retries, buffers and actions. Validate counts, indices and inputs. Preserve acquisition time and use wrap-safe elapsed-time calculations.
- `volatile` is for required hardware/asynchronous access; it is not synchronization. Share data only through the three [ARCHITECTURE](ARCHITECTURE.md#shared-data-rules) patterns: one-word snapshot by single store/load, mutex copy above 32 bits, ISR-to-task ring.
- Every storage unit has one writer; only that writer performs read-modify-write on it. Packed data shared with an ISR on core 1 is updated inside `DI`/`EI`; packed data written from both cores needs a mutex or an RP2040 hardware spinlock (`DI` masks only the local core).
- Keep ISRs bounded: capture, signal, return. No blocking, formatting, decoding or allocation in ISR. Use `_task` / `_isr` names where applicable.
- Respect [TEAM ownership](TEAM.md#3-peripherals): one motor writer, separate peripheral owners, kernel-only networking. Stop must not depend on diagnostics queues or the request mutex.
- Keep credentials out of version control. Track behavior and evidence through the owning guides and [test records](../testing/README.md).

## Minimum-bit data

- Give each application field the fewest bits that hold its full valid range plus any invalid code: `ceil(log2(number of values))`. Record range, unit, signedness, scale, rounding, invalid code, overflow behaviour and width in [DATA_REPRESENTATION](DATA_REPRESENTATION.md) before converting it.
- Mutually exclusive states use one encoded value; faults that can coexist use independent bits.
- Combine small fields into one storage unit only when they share a writer.
- Access packed fields only through the owning module's accessor functions (`module_field_get()` / `module_field_set()`); no caller repeats masks or shifts.
- Loop counters and short-lived temporaries are exempt.
- Test every packed field at its minimum, maximum and invalid code, and check that overflow saturates or faults as documented.

## Review checklist

- [ ] Purpose, API, units and ownership are clear; no duplicate mutable state.
- [ ] Braces, spacing, names and constants are consistent.
- [ ] Inputs, casts, arithmetic, finite values and array bounds are checked.
- [ ] Hardware operations/actions are bounded; failures inhibit unsafe output.
- [ ] Shared values are synchronized; cached observations retain acquisition time.
- [ ] Stop, stale data and reset preserve the documented contract.
- [ ] No ISR blocking, foreign motor writes, student network calls or committed credentials.
- [ ] Packed fields match DATA_REPRESENTATION, use accessors, and have boundary tests.
- [ ] Each shared storage unit has exactly one writer.
- [ ] Evidence identifies source/profile; a build is not hardware proof.
