---
name: cpp-dev
description: Use BEFORE modifying any KTPAMXX C++ (core, DODX, includes) — fork-delta discipline, the extension-mode init-parity trap, game-thread I/O rules, build/smoke-test/stage workflow, and verify-by-md5. Also use when planning a change, to know which invariants it touches.
---

# KTPAMXX Development

This is a fork of AMX Mod X running in **extension mode** (loaded directly by
KTP-ReHLDS — no Metamod) on a 24-instance production fleet. The rules below each
encode a real production incident; follow them even when they feel paranoid.

## Hard safety rules
- **NEVER restart game servers** without the operator's explicit permission in
  the current conversation.
- Binaries deploy as `.new` files (`dlls/ktpamx_i386.so.new`,
  `modules/dodx_ktp_i386.so.new`) and swap at the 03:00 ET nightly restart.
- **Commit source before or at ship.** Shipped-but-uncommitted source has
  happened (2.7.22) and is standing debt — never widen that gap.

## Fork-delta discipline
Only touch KTP-owned code. Upstream AMX Mod X code is out of scope for cleanup,
refactor, or "optimization" — changing it breaks merge-ability and risks behavior
nobody has audited. Identify KTP code via git history, `KTP`/`ktp` markers, and
CHANGELOG.md. When a fix genuinely requires editing an upstream file, keep the
diff minimal and flag it in the CHANGELOG entry.

## The extension-mode parity trap (most important rule in this file)
Upstream AMXX assumes Metamod: `Meta_Attach`, `C_Spawn`, `Meta_Detach` etc. In
extension mode **none of those run** — extension init/shutdown re-implements
them, and every divergence is a latent crash:
- `hostname` cvar_t* was assigned only in `C_Spawn` → NULL-deref on
  `get_user_name(0)` in extension mode (DAL1 crash, fixed in 2.7.22).
- Module detach was never called at shutdown (Meta_Detach is Metamod-only) →
  fixed via the ReHLDS `KTP_ExtensionShutdown` callback (2.7.21 + .928).

**When touching any init, shutdown, or per-map lifecycle code:** enumerate what
the Metamod path sets/tears down and verify the extension path does the same.
Every global assigned in a Metamod-only function is a candidate NULL-deref.
Remember `CLog::MapChange()` runs per map in extension mode only as of 2.7.19 —
per-map behaviors need explicit extension-mode wiring.

## Game-thread rules
- **No blocking I/O on the game thread.** Per-line fopen/fprintf/fclose logging
  caused 100ms+ frame stalls on ext4 journal commits during live matches. All
  logging goes through the async CLog writer (2.7.19); don't add new synchronous
  file/network calls reachable from the frame loop.
- The async writer pattern: game thread enqueues into a bounded ring
  (drop-with-counter, never blocks); a writer thread owns the FILE* (line-
  buffered via setvbuf for crash durability). Kill switch: `amxx_log_async`
  localinfo (default 1). Follow this pattern for any new async work.
- No exception-dependent error paths in code that runs against the engine —
  ReHLDS builds `-fno-exceptions`, so nothing may throw across that boundary.
  Use the pthread_create-with-sync-fallback pattern, not std::thread.
- Plugin-visible counters and lifecycle state are process-wide, not per-map.
  CTask's `m_ActiveCount` double-decrement (task self-removal) silently stalled
  ALL `set_task` timers fleet-wide — audit increment/decrement symmetry on any
  counter you touch.

## DODX specifics
- `g_observedDeaths` and similar per-match state must reset on the extension-mode
  lifecycle (upstream `Connect()`-based resets are unreachable) — see the 2.7.20
  score-persistence fixes before touching death/stat counting.
- Death dedup must stay exactly-once and symmetric; HLStatsX consumes the output.
- The `dod_client_weapon_fire` forward and `dodx_test_dispatch_*` natives are
  **deliberately dormant** (pending consumers / Tier-2 test drivers). Do not
  remove them as dead code.
- `plugins/include/*.inc` is the authoritative include set for EVERY KTP plugin.
  Changing an .inc (especially forward signatures) requires recompiling dependent
  plugins — KTPHLTVRecorder depends on KTPMatchHandler forwards, and all plugins
  compile against these includes.

## Workflow
1. **Build**: `wsl bash -c "cd '/mnt/n/Nein_/KTP Git Projects/KTPAMXX' && bash build_linux.sh"`
   (AMBuild → `obj-linux/packages/`, auto-stages to the KTP DoD Server test tree).
   WSL rebuild gotchas after an env reinstall: nasm may be missing, a stale
   `obj-linux` built against newer glibc must be wiped, git `safe.directory`, and
   this repo needs autocrlf handled (CRLF damage breaks the build).
2. **Review**: `ktp-code-review` agent before staging anything.
3. **Smoke test on the Tier-2 runner** (data server, `/opt/ktp-tier2-runner`):
   before/after comparison for crash-class fixes (boot → map change → quit,
   exit 0, ordered log rotation). The runner's module stack must match the fleet;
   sync is deliberate, never automated.
4. **Fleet stage**: `.new` via paramiko to all 24 active instances; md5-verify
   every staged file.
5. **Post-activation verify**: 24/24 on the new md5, no leftover `.new`, zero new
   cores — check `/tmp` (`find /tmp -maxdepth 1 -name 'core.*' -mtime -1`), NOT
   the game trees (that search matches only core.so/core.ini/core.wav and always
   looks clean). **Verify deployments by md5, never by the console banner** —
   appversion commit-counts skew vs CHANGELOG titles.

## Versioning
Bump the version for every shipped change and write the CHANGELOG.md entry with
what/why + the md5 of the shipped binary once built. Consoles stamp
`MAJOR.MINOR.PATCH.BUILD`; the build number comes from commit count, which is why
banners can't be trusted for deploy verification.
