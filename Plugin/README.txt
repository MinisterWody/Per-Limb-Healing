PerLimbHealing native plugin source
===================================

Target: Fallout 4 OG/NG/AE desktop runtime families / F4SE 0.6.23 and 0.7.x
Plugin version: 52

Build characteristics
- CRT-free clang-cl/lld-link build.
- No CRT/STL/CommonLib binary dependency is linked.
- Requires the user-installed matching Address Library database selected from the detected runtime.
- Production compilation uses /W4 and must complete with zero warnings.
- Deterministic link uses /timestamp:0.

Current runtime architecture
- Exact patch numbers are diagnostic information, not an allow/deny list.
- The DLL exports classic F4SEPlugin_Query for OG and modern F4SEPlugin_Version metadata for NG/AE, plus one shared F4SEPlugin_Load implementation.
- A runtime-family profile owns Address Library IDs plus only ABI layout/virtual-slot facts Address Library cannot provide.
- Pre-980 1.10.x selects the OG candidate; 1.10.980+ selects NG; 1.11.x selects AE. Exact runtime validation status is informational and capability checks remain authoritative.
- Canonical Fallout forms and ActorValueInfo records are resolved through Address-Library-backed TESForm::GetFormByNumericID rather than ActorValue/TESDataHandler structure traversal.
- OG 1.10.163, NG 1.10.980/1.10.984 and AE 1.11.137/1.11.191/1.11.221/1.11.240 were statically qualified during development; 1.11.221 was additionally gameplay-validated.
- All three families use one PLH-relevant desktop EngineLayout backed by direct RTTI/vtable evidence, direct AE disassembly, recovered OG code evidence and cross-runtime source analysis performed during development.
- Address Library resolution records scan completion plus found/usable/required identity counts. Passive-gate, Stimpak and Scaleform address capabilities are evaluated separately rather than treating a completed database pass as universal success.
- Stimpak target Actor, selected Health ActorValue, and condition scale are obtained through Address-Library-validated Bethesda virtual helpers rather than raw +0x98/+0xB0 reads.
- Scaleform GFxMovieView/movieRoot/CreateFunction ABI reads are checked with VirtualQuery and Fallout-image/executable provenance before the indirect call.
- Exact runtimes outside the binary-qualified project corpus must successfully create one hardened Scaleform bridge before the Stimpak backend is allowed to install. Qualified runtimes retain the existing pre-Pip-Boy activation behavior.
- Player-only ConditionRateMult ownership and MCM synchronization remain unchanged.

Diagnostics
- Fresh-install Debug Log default is Errors, not Off.
- Errors records a compact operational startup/capability summary plus failures/abnormal states.
- Verbose adds normal lifecycle/state diagnostics and per-setting changes.
- Performance Log remains independently opt-in and zero-tick QPC samples still contribute to call counts.

Source-comment policy
Production comments document current invariants, ABI constraints, engine behavior and safety rationale.

Build
Run build_plugin.cmd from an x64 environment with clang-cl and lld-link available.
The script can be launched from any working directory and writes the canonical production DLL to Built\PerLimbHealing.dll while keeping compiler intermediates under build/.
No Stimpak healing hot path performs settings-file or log-file I/O.

Log lifecycle
PerLimbHealing.log is written to the user Documents known folder under My Games\Fallout4\F4SE. The first diagnostics open in each Fallout process replaces any previous-session log; later same-process reopens append.
