# Project C coding standard

Apply these rules to new or changed application code. They are a concise project adoption informed by Barr Group's 2018 embedded C guidance; they do not certify complete compliance or require reformatting the imported kernel.

## Language and layout

- The build uses **GNU C11** (`-std=gnu11`). Retain C11 atomics for cross-core startup publication; isolate other necessary compiler/hardware extensions in drivers and document them.
- Use four spaces, LF endings, one statement per line and braces for every control body. Put opening braces on their own line; aim for 80-character lines.
- Space binary operators, clarify precedence with parentheses, and comment purpose, units, bounds and assumptions. Remove unused/commented-out code.

## Modules and data

- Give each module one purpose and matching lowercase `snake_case.c` / `.h` names. Include its own guarded header; expose only needed declarations. Never include a `.c` file or absolute include path.
- Prefix public functions with the module name; make private functions/data `static`. Use descriptive names and named constants. Prefer functions to macros; required macros evaluate arguments once and parenthesize expressions.
- Use fixed-width integers where width matters, `bool` for flags and `const` for immutable data. Keep unsigned literals/ranges consistent; justify narrowing casts and avoid signed bit operations.
- Initialize data before use; represent unavailable values explicitly. New types may use module-prefixed `snake_case_t`; established public names such as `IrRaw` and `MotionRequest` remain compatible until an agreed refactor.
- Use `float` only where justified, `f` suffixes and finite/range checks. Do not serialize native structs or rely on padding/bit-field order. Unsupported float typedef names are not required.

## Control and concurrency

- Keep functions reviewable; check return codes and choose safe failure responses. Include `default` in switches and document intentional fallthrough. Early error returns are acceptable.
- Bound hardware polling, waits, retries, buffers and actions. Validate counts, indices and inputs. Preserve acquisition time and use wrap-safe elapsed-time calculations.
- `volatile` is for required hardware/asynchronous access; it is not synchronization. Use documented locks/atomics and copy coherent snapshots.
- Keep ISRs bounded: capture, signal, return. No blocking, formatting, decoding or allocation in ISR. Use `_task` / `_isr` names where applicable.
- Respect [TEAM ownership](TEAM.md#3-peripherals): one motor writer, separate peripheral owners, kernel-only networking. Stop must not depend on diagnostics queues or the request mutex.
- Keep credentials out of version control. Track behavior and evidence through the owning guides and [test records](../testing/README.md).

## Review checklist

- [ ] Purpose, API, units and ownership are clear; no duplicate mutable state.
- [ ] Braces, spacing, names and constants are consistent.
- [ ] Inputs, casts, arithmetic, finite values and array bounds are checked.
- [ ] Hardware operations/actions are bounded; failures inhibit unsafe output.
- [ ] Shared values are synchronized; cached observations retain acquisition time.
- [ ] Stop, stale data and reset preserve the documented contract.
- [ ] No ISR blocking, foreign motor writes, student network calls or committed credentials.
- [ ] Evidence identifies source/profile; a build is not hardware proof.
