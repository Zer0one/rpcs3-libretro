# Upstream Integration Policy

The original Libretro port is the reference base for this fork. Adapt this fork's improvements and customizations to that base as it evolves. Keep differences limited, recognizable and justified so they can be reviewed for a future contribution to the original port.

## Separate References

| Reference | Repository | Role | Local Remote |
| --- | --- | --- | --- |
| Original Libretro port | [WizzardSK/rpcs3-libretro](https://github.com/WizzardSK/rpcs3-libretro) | Core integration, frontend contracts and port architecture | `libretro` |
| Standalone emulator | [RPCS3/rpcs3](https://github.com/RPCS3/rpcs3) | Emulation engine and matching standalone comparison | `upstream` |
| This fork | [Zer0one/rpcs3-libretro](https://github.com/Zer0one/rpcs3-libretro) | Local improvements, customizations and numbered releases | `origin` |

The `libretro` remote and this fork's local `libretro` branch are distinct Git references. Identify repositories and exact revisions in integration reports rather than using "upstream" ambiguously. Remote names describe this checkout's configuration.

Development iterates on both upstream references: the original Libretro core for the port and upstream standalone RPCS3 for the engine and behavioral comparisons. Local adaptations belong to this core. Do not modify standalone to match this fork unless the maintainer explicitly requests that work. Matching engine versions do not imply identical behavior when this core contains a documented local correction.

## Adapting Local Changes

- Follow changes to APIs, structures and formats in the original port. Do not preserve a historical fork representation solely because local code already uses it; adapt that code to the updated reference.
- Retain an exception only for a concrete defect or documented requirement. Record the affected upstream revision, reason, scope, evidence and condition for removing the exception near the adaptation or in a linked project document.
- Review each local improvement against the updated base. Preserve its intended benefit, remove it when upstream supplies an equivalent solution, and avoid keeping duplicate implementations.
- Use focused changes and clear comments where the adaptation is not self-evident. Explain the behavior and reason for a difference; avoid unrelated refactoring that makes later comparison harder.
- When a format change affects existing user data or saved options, describe the migration and compatibility impact. Following upstream does not justify silently discarding user data.

Use existing code comments, focused technical documents and Git history to explain local differences; do not maintain a duplicate patch registry or a manual committed/uncommitted inventory. Keep the explanation and its validation limits current with the implementation. For example, framebuffer retirement is documented in [the Async lifetime investigation](../MACOS_PERFORMANCE.md#async-framebuffer-lifetime-investigation), while runtime configuration, enabled-device limits and pause integration are covered by [the GPU configuration audit](GPU_CONFIGURATION_AUDIT.md).

## Integration And Review

Before an authorized integration, compare exact source revisions, identify overlapping local changes and check relevant contracts and formats. A policy update alone does not authorize a merge, release or upstream pull request.

For either upstream, review incoming changes against this fork's existing adaptations in the affected areas, including related resource lifetimes, configuration and frontend contracts. Review semantic overlap even when Git reports a clean merge. Do not treat the entire diff against the Libretro donor as locally authored: it can also contain newer standalone-engine changes. Preserve the intended local behavior by adapting it to the new upstream implementation; remove a local correction only after verifying that upstream supplies an equivalent solution or that its documented need no longer applies. Update the existing explanation and rerun the relevant checks when the implementation or its assumptions change. Record which of those decisions was taken in the integration change, so a correction cannot silently disappear or survive as a duplicate.

Prepare a concrete proposal for Andrea's review when a decision involves substantial behavior changes, incompatible migration, loss of a local feature or a significant architectural compromise. State the alternatives, evidence, user impact and recommended choice before implementing the dependent change. Routine adaptations within the authorized scope follow this policy without a separate approval step.

RPCS3 engine updates continue to follow [AGENTS.md](../AGENTS.md): use full upstream merges, immediately apply `.upstream-excluded` before resolving other conflicts, and do not restore excluded standalone components. Update `rpcs3/libretro/upstream.version` and `rpcs3/libretro/config_database.dat` together with the engine merge. Preserve this fork's changelog. These engine rules are separate from following the original Libretro port.

Validate the affected behavior and record the source identities. For suspected engine failures, compare standalone RPCS3 at the same engine build before attributing the failure to the core. Keep build, frontend load and gameplay evidence distinct, and respect any active limits on runtime testing.

Use the latest available standalone release as the normal test reference and update the core's engine to match it when necessary. Test older versions only for a concrete diagnostic need, such as identifying a regression. Update the existing standalone installation through its dedicated project chat instead of creating additional isolated application copies; preserve user data and record the exact executable and driver versions used.

`AGENTS.md` and `CLAUDE.md` remain developer-owned. This document supplements their rules without changing protected files or rewriting published history.
