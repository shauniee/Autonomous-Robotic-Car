# CSC01 robot car

## Contents

- [Language](#language)
  - [Requirements and evidence](#requirements-and-evidence)
  - [Planning and runtime terms](#planning-and-runtime-terms)

Shared vocabulary for the INF2004 CSC01 line-following robot car project and its course requirements.

## Language

### Requirements and evidence

**Minimum-bit representation**:
The smallest data representation of a field that is justified by its documented bounds, units, signedness, scale, rounding and invalid codes.
_Avoid_: Bit packing, compression, compact format

**Final implementation**:
The firmware as it stands after the additional requirements are implemented; the only implementation that evidence is required for.
_Avoid_: After, revised implementation

**Baseline**:
A historical measurement of the implementation before the minimum-bit change; retained as a record and not required.
_Avoid_: Before, original implementation

**Evidence**:
A recorded result tied to an exact commit and test ID that shows the final implementation meets a stated pass criterion.
_Avoid_: Proof, comparison

### Planning and runtime terms

[Plan](plan.md) owns the work sequence; [architecture](docs/architecture.md) owns
runtime contracts. A subsystem is a module/responsibility, not necessarily a task.
The target has three application tasks plus existing kernel tasks. Mission has
navigation authority; Motion has higher scheduling priority and sole motor-output
authority. A new run follows an accepted local start; a stop/reset never starts
movement. Report-only data loss is distinct from insufficient evidence for a
specific manoeuvre.
