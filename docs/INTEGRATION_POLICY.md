# Upstream Integration Policy

The original Libretro port is the reference base for this fork. Adapt this fork's improvements and customizations to that base as it evolves. Keep differences limited, recognizable and justified so they can be reviewed for a future contribution to the original port.

## Separate References

| Reference | Repository | Role | Local Remote |
| --- | --- | --- | --- |
| Original Libretro port | [WizzardSK/rpcs3-libretro](https://github.com/WizzardSK/rpcs3-libretro) | Core integration, frontend contracts and port architecture | `libretro` |
| Standalone emulator | [RPCS3/rpcs3](https://github.com/RPCS3/rpcs3) | Emulation engine and matching standalone comparison | `upstream` |
| This fork | [Zer0one/rpcs3-libretro](https://github.com/Zer0one/rpcs3-libretro) | Local improvements, customizations and numbered releases | `origin` |

The `libretro` remote and this fork's local `libretro` branch are distinct Git references. Identify repositories and exact revisions in integration reports rather than using "upstream" ambiguously. Remote names describe this checkout's configuration.

## Adapting Local Changes

- Follow changes to APIs, structures and formats in the original port. Do not preserve a historical fork representation solely because local code already uses it; adapt that code to the updated reference.
- Retain an exception only for a concrete defect or documented requirement. Record the affected upstream revision, reason, scope, evidence and condition for removing the exception near the adaptation or in a linked project document.
- Review each local improvement against the updated base. Preserve its intended benefit, remove it when upstream supplies an equivalent solution, and avoid keeping duplicate implementations.
- Use focused changes and clear comments where the adaptation is not self-evident. Explain the behavior and reason for a difference; avoid unrelated refactoring that makes later comparison harder.
- When a format change affects existing user data or saved options, describe the migration and compatibility impact. Following upstream does not justify silently discarding user data.

## Integration And Review

Before an authorized integration, compare exact source revisions, identify overlapping local changes and check relevant contracts and formats. A policy update alone does not authorize a merge, release or upstream pull request.

Prepare a concrete proposal for Andrea's review when a decision involves substantial behavior changes, incompatible migration, loss of a local feature or a significant architectural compromise. State the alternatives, evidence, user impact and recommended choice before implementing the dependent change. Routine adaptations within the authorized scope follow this policy without a separate approval step.

RPCS3 engine updates continue to follow [AGENTS.md](../AGENTS.md): use full upstream merges, immediately apply `.upstream-excluded` before resolving other conflicts, and do not restore excluded standalone components. Update `rpcs3/libretro/upstream.version` and `rpcs3/libretro/config_database.dat` together with the engine merge. Preserve this fork's changelog. These engine rules are separate from following the original Libretro port.

Validate the affected behavior and record the source identities. For suspected engine failures, compare standalone RPCS3 at the same engine build before attributing the failure to the core. Keep build, frontend load and gameplay evidence distinct, and respect any active limits on runtime testing.

`AGENTS.md` and `CLAUDE.md` remain developer-owned. This document supplements their rules without changing protected files or rewriting published history.
