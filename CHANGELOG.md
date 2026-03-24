# Changelog

All notable changes to KTP AMX will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.7.4] - 2026-03-24

### Fixed

#### Message Hook RemoveHook Wrong Index (messages.h)
`RegisteredMessage::RemoveHook` called `m_Forwards.remove(forward)` where `forward` is the SP forward ID value. `ke::Vector::remove(size_t)` treats the argument as a position index, so this removed at position `forward` (a random index) instead of position `i` (the matched entry). Stale forward IDs accumulated in the hook vector on every map change cycle of `register_message`/`unregister_message`. Fixed to use `m_Forwards.remove(i)`.

#### Client_ObjScore Stale Player Pointer (DODX usermsg.cpp)
`Client_ObjScore` used a `static CPlayer*` across message parse states without revalidation at case 1. If the edict was freed between case 0 and case 1 (possible during extension mode message dispatch), `pPlayer->savedScore` would read corrupt memory. Added validity recheck (`ingame`, `pEdict`, `pEdict->free`) at the top of case 1.

#### PreThink Fallback Init Removed (DODX moduleconfig.cpp)
`DODX_OnPlayerPreThink` had a fallback `g_pFirstEdict` initialization using `ENTINDEX()` that could run if `DODX_OnSV_ActivateServer` failed. `ENTINDEX` is an engine call that may not be safe during early-stage init. Replaced with a hard guard (`if (!g_pFirstEdict) return;`) since `DODX_OnSV_ActivateServer` is the proper init path.

#### CPlayer::Disconnect Missing Edict Free Check (DODX CMisc.cpp)
`Disconnect()` called `ignoreBots(pEdict)` without first checking if the edict was freed. During crash/map-change sequences, `pEdict->free` can be set before `ClientDisconnect` fires, causing `ignoreBots` to dereference freed entity flags. Added `if (!pEdict || pEdict->free) return;` guard.

#### Event/LogEvent Dedup O(n) Reverse-Lookup Eliminated (CEvent.cpp, CLogEvent.cpp)
Event and log event dedup scanned the full `EventHandles`/`LogEventHandles` table to recover the handle ID for a duplicate registration. Added `m_HandleId` field to `ClEvent` and `CLogEvent`, cached at creation time, enabling O(1) handle lookup during dedup.

#### Rank Save Skipped in Extension Mode (DODX moduleconfig.cpp)
`ServerDeactivate` called `g_rank.saveRank()` unconditionally, performing unnecessary file I/O in extension mode where the rank system is unused. Added `if (!g_bExtensionMode)` guard.

---

## [2.7.2] - 2026-03-13

### Fixed

#### CLogEvent Second Overload Last-Char Trim (CLogEvent.cpp)
The variadic `setLogString` overload still had the old `logString[--len] = 0` trim that was correctly removed from the first (va_list) overload. Every log event written through `AlertMessage_RH` in extension mode had its last character silently dropped (typically the closing `"` on DoD log events), which could break `register_logevent` filter matching.

#### MessageHook_Handler Null Chain Propagation (meta_api.cpp)
When `msg` was null, the handler called `chain->callNext(null)` which would propagate null into downstream hooks that dereference `msg`. Changed to return immediately without calling the chain.

#### Say/Say_team Prefix List Separation (meta_api.cpp)
Registered `say_team` prefix before `say` to prevent prefix list merging. `findPrefix` uses `strncmp` with the prefix's own length, so `"say"` (len 3) matched `"say_team"` causing both to share one list (~199 entries). Now separated: ~119 `say` + ~80 `say_team`.

---

## [2.7.1] - 2026-03-13

### Critical — SP Forward Null Deref (CForward.cpp)
`CSPForward::execute` crashed if `findPluginFast(m_Amx)` returned null (plugin unloaded while forward handle still live). Added null check before dereference.

### Critical — MessageHook_Handler Null Check Order (meta_api.cpp)
Null check on `msg` occurred after `chain->callNext(msg)` — crash if msg was null inside the chain. Moved null check before callNext.

### Critical — Extension Mode Map-Change Memory Leaks (meta_api.cpp)
`KTPAMX_ReloadPlugins` was missing `g_xvars.clear()`, `g_vault.clear()`, and `ClearPluginLibraries()`. Xvar IDs accumulated without dedup each map change (stale cross-plugin variable access). Plugin-registered native trampolines leaked mmap'd memory per map change.

### Critical — DODX `dod_weaponlist` Array OOB (NBase.cpp)
Bounds check used `WEAPONLIST` (hardcoded 71) but the `weaponlist[]` array only has 42 entries. Indices 42-70 accessed uninitialized memory. Replaced with `WEAPONLIST_SIZE` computed from actual array size.

### Critical — Event `parserInit` Off-By-One (CEvent.cpp)
Guard used `msg_type > MAX_AMX_REG_MSG` (should be `>=`). Allowed access one past end of `m_Events[]` array.

### Fixed

#### DODX Shot Double-Counting Disabled (CMisc.cpp)
`CheckShotFired()` button-based shot detection ran in PreThink alongside `CurWeapon` message handler clip-decrement detection. Every shot was counted twice in extension mode, inflating HLStatsX accuracy stats. Disabled button-based path — CurWeapon handler is authoritative in both modes.

#### `SV_CheckConsistencyResponse_RH` Player Guard (meta_api.cpp)
Added `pPlayer->initialized` check before firing `FF_InconsistentFile` forward. Prevents plugin handlers from accessing uninitialized player state during connection handshake.

#### `dodx_give_grenade` Entity Leak (NBase.cpp)
`oldSolid` was captured before `pfnSpawn` which changes `.solid`. Post-touch comparison was always false, so failed pickups never cleaned up the entity. Moved capture to after spawn.

#### `_FORTIFY_SOURCE=2` Debug Build Fix (AMBuildScript)
Moved `-D_FORTIFY_SOURCE=2` into the optimization block. GCC requires `-O1+` for FORTIFY; without it, `-Werror` fails debug builds.

#### `CLogEvent` Truncation Handling (CLogEvent.cpp)
Used full `sizeof(logString)` instead of hardcoded 255. Fixed POSIX truncation detection (returns would-be length, not -1). Removed unnecessary last-character trim.

#### `C_ClientConnect_Post` Bounds Check (meta_api.cpp)
Added ENTINDEX bounds check before `GET_PLAYER_POINTER`. Prevents crash if Metamod passes entity 0 or out-of-range entity.

#### `DODX_OnMsgBegin` gpGlobals Guard (moduleconfig.cpp)
Added null guard on `gpGlobals` before accessing `->maxClients`. Prevents crash if message fires before DODX extension hooks are initialized.

#### `CALMFromFile` sscanf Width (CPlugin.cpp)
`sscanf("%s")` into `pluginName[256]` changed to `"%255s"`.

#### `srvcmd.cpp` strtol Validation (srvcmd.cpp)
`strtol` end-pointer check was dead code (`!pEnd` — strtol never returns NULL). Fixed to check for no-digits-parsed and trailing garbage.

## [2.7.0] - 2026-03-13

### Critical — JIT/ASM32 Re-Enabled
The Pawn JIT compiler and x86 ASM dispatcher were disabled since the initial KTP fork (`AMBuilder` lines 7-9 commented out with "KTP DEBUG" label). All plugins were executing through the slow C interpreter instead of native x86 JIT-compiled code. Re-enabled `JIT`, `ASM32` defines and `amxexecn.asm`, `amxjitsn.asm` assembly files. Significant performance improvement expected for all plugin callbacks.

### Critical — Security Hardening Flags
Added compiler and linker hardening flags to `configure_linux` in `AMBuildScript`:
- `-fstack-protector-strong` — stack canary protection for local buffers
- `-D_FORTIFY_SOURCE=2` — compile-time and runtime bounds checking on libc functions
- `-Wl,-z,relro -Wl,-z,now` — full RELRO (GOT read-only after dynamic linking)

### Critical — Module SDK `rewriteNativeLists` Double-Free (CModule.cpp)
`MNF_OverrideNatives` called more than once (multiple modules across map loads) appended the same index to `m_DestroyableIndexes` without dedup. On `clear()`, the destructor loop called `delete[]` on the same index twice. Added dedup check before appending.

### Critical — `detachReloadModules` Stale Pointers (modules.cpp)
After `detachReloadModules()`, `g_moduleFrameCallbacks` and the three message handler arrays (`g_ModuleMsgBeginHandlers`, `g_ModuleMsgHandlers`, `g_ModuleMsgEndHandlers`) retained function pointers into unmapped `.so` memory. Added `Module_ClearFrameCallbacks()` and `Module_ClearMsgHandlers()` cleanup functions called from `detachReloadModules`.

### Critical — DODX `saveKill`/`saveHit` Bounds Checks (CMisc.cpp)
`wweapon` used as index into `weapons[]`, `weaponsLife[]`, `weaponsRnd[]`, `weaponData[]` without validation. `bbody` (hitgroup) indexed `bodyHits[8]` without bounds check. Added clamping: `wweapon` to `[0, DODMAX_WEAPONS)`, `bbody` to `[0, 7]`.

### Critical — DODX `Client_CurWeapon` Bounds Check (usermsg.cpp)
`iId` from network message used as direct array index into `weaponData[]` and `weapons[]` without range check. Added `break` guard for out-of-range values.

### Critical — `C_ClientCvarChanged` Missing Guard (meta_api.cpp)
`pfnClientCvarChanged` fired `client_cvar_changed` forward without checking `pPlayer->initialized` or `pPlayer->ingame`. Cvar responses during reconnect/map-change could crash plugin handlers. Added `initialized && ingame` guard before `executeForwards`.

### Fixed

#### Pass-Through Hooks Disabled (meta_api.cpp)
`ExecuteServerStringCmd_RH` and `Steam_GSBUpdateUserData_RH` were registered unconditionally but did nothing except call the chain. Every client command went through extra function call overhead for zero functionality. Commented out registration and function definitions.

#### `sscanf` Unbounded Writes (CCmd.cpp, CPlugin.cpp)
`Command::Command()` used `sscanf("%s %s")` into `char[64]` buffers — changed to `"%63s %63s"`. `loadPluginsFromFile` used `"%s"` into `char[256]` — changed to `"%255s"`.

#### `C_StartFrame_Post` Dead Code Guard (meta_api.cpp)
The `g_putinserver` processing block in `C_StartFrame_Post` was dead code in extension mode. Replaced `#if 0` with runtime guard `!g_bRehldsExtensionInit` so it only runs under Metamod.

#### `g_szPreviousMap` Dead Variable Removed (meta_api.cpp)
Written in three places, read nowhere. Removed declaration and all write sites.

#### Module SDK Null/Free Edict Guards (modules.cpp)
`MNF_GetPlayerFrags`, `MNF_GetPlayerHealth`, `MNF_GetPlayerArmor`, `MNF_IsPlayerHLTV` accessed `pPlayer->pEdict->v` without null/free checks. Added guards returning 0 on null or freed edicts.

#### `sprintf` Overflow in `CheckModules` (modules.cpp)
`sprintf(error, ...)` into `char[128]` with no length limit. Changed to `snprintf(error, 128, ...)`.

#### DODX `get_user_wrstats` Wrong Guard (NRank.cpp)
Guard checked `pPlayer->weaponsLife[weapon].shots` (life stats) but function copies from `pPlayer->weaponsRnd[weapon]` (round stats). Changed guard to `weaponsRnd`.

#### DODX `Scoping()` Global vs `this` (CMisc.cpp)
Member function `CPlayer::Scoping()` accessed `mPlayer->current` (global message state pointer) instead of `this->current`. Fixed all three references.

#### DODX `cwpn_dmg` Dead Null Check (NBase.cpp)
`pAtt` dereferenced before null check — guard was unreachable. Removed dead `if(!pAtt) pAtt = pVic` block.

#### Forward Dedup Missing Param Types (CForward.cpp, CForward.h)
Multi-forward dedup matched on name + execType + numParams only. Two forwards with same name/count but different param types would collide. Added `memcmp` on `m_ParamTypes` array to match SP forward dedup behavior.

#### Dual `g_putinserver` Processing Guard (meta_api.cpp)
Both `C_StartFrame_Post` and `SV_Frame_RH` could process the `g_putinserver` queue in extension mode. `C_StartFrame_Post` now guarded to only run under Metamod.

### Build System
- 32-bit architecture check changed from warning to hard `exit 1` (`build_linux.sh`)
- Removed dead deploy blocks for fun/engine/fakemeta modules (`build_linux.sh`)
- Removed fun/engine/fakemeta from CLAUDE.md build output table

## [2.6.18] - 2026-03-12

### Fixed

#### DODX Module - Pdata Detection Log Spam
`DODX_PdataWriteBoth` and `DODX_DetectPdataOffset` called `MF_Log` (synchronous `fprintf`) on every grenade set call during the detection phase. On map load with KTPPracticeMode active and players spawning, this generated dozens of synchronous log writes during an already high-load window. Changed Phase 1 to log once via `MF_PrintSrvConsole`, removed per-call Phase 2 probe logging, and kept only the final detection result output.

#### DODX Module - Stickgrenade-First Detection Failure
`DODX_DetectPdataOffset` only probed handgrenade offsets, but Phase 1 writes go to whichever grenade family was requested. If the first `dodx_set_grenade_ammo` call was for stickgrenades (Axis players), the handgrenade offsets were never written, causing detection to read uninitialized data and defer indefinitely. Now probes both handgrenade and stickgrenade families, scoring across all 6 locations.

#### DODX Module - Tied Score Tiebreaker Simplification
The old consistency tiebreaker compared per-location value equality, but those local variables no longer exist after the multi-family probe rewrite. Simplified to default to +4 on tied scores, which matches the historical Ubuntu 22.04 behavior.

#### Stats Logging - Flush Task Registration in Hookchain Context
`stats_logging.sma` registered its repeating buffer flush task in `plugin_init`, which fires from within a ReHLDS hookchain handler in extension mode. `set_task` with repeating flag intermittently fails to register in this context (~10% failure rate). Moved task registration to `plugin_cfg`, which fires later and outside the hookchain. Without this fix, headshot kill events could accumulate in the buffer and never flush until the next map change.

#### Core AMXX - Dead Code Removal
Removed `KTPAMX_ServerDeactivate()` and `KTPAMX_ServerDeactivatePost()` — superseded by `SV_InactivateClients_RH` hook in v2.6.15 but left in the file. The old functions contained an older disconnect loop that manually zeroed player fields instead of calling `pPlayer->Disconnect()`, creating a maintenance hazard.

## [2.6.17] - 2026-03-11

### Fixed

#### DODX Module - Team Score Zeroed Before Halftime Save

`DODX_OnChangelevel` reset `AlliesScore` and `AxisScore` to 0 before KTPMatchHandler's changelevel hook could read them. This caused `save_first_half_scores()` to always save 0-0, breaking score carry-forward into the second half. Every match since v2.6.15 reported 0-0 at halftime regardless of actual score.

Moved score zeroing from `DODX_OnChangelevel` (pre-changelevel) to `DODX_OnSV_ActivateServer` (post-map-load), so plugin hooks can read the actual scores during the changelevel transition before they're cleared for the new map.

## [2.6.16] - 2026-03-11

### Fixed

#### DODX Module - Pdata Offset Auto-Detection Rewrite

The pdata offset auto-detection for grenade ammo operations failed on first player spawn because the game DLL hadn't initialized the player's private data yet. The old logic required all 3 grenade memory locations to contain matching valid values (1-10), but at spawn time they were uninitialized (all zeros), causing detection to fail silently and lock in the wrong default (+4).

Replaced with a two-phase write-then-verify approach:
- **Phase 1** (first grenade set call): Writes the requested count to BOTH +4 and +5 offsets, ensuring the correct one gets the right value regardless of which is correct
- **Phase 2** (second grenade set call): Reads back from both offsets and scores each by how many of the 3 locations contain valid values (1-10). The higher-scoring offset wins, with a minimum threshold of 2/3
- If neither offset has sufficient data, detection defers and retries on the next grenade operation instead of locking in a wrong answer

This eliminates the need for manual `pdata_offset` configuration in `dodx.ini` — the correct offset is determined automatically based on the actual game DLL binary and server environment.

## [2.6.15] - 2026-03-11

### Fixed

#### Core AMXX - Extension Mode Lifecycle Gaps

`plugin_end` and `client_disconnect` forwards were not firing before map transitions in extension mode. This caused memory leaks and missing cleanup on every map change:
- `modules_callPluginsUnloading()` now called before plugin re-initialization, allowing modules (ReAPI) to clear hookchain vectors that are 100% plugin-owned
- Data handles (Arrays, Tries, DataPacks) properly freed between maps
- HUD sync objects, dynamic admins, and cvar manager state cleaned up
- Grenade and auth caches cleared on reload

#### Core AMXX - Plugin Re-initialization Deduplication

Subsystem registrations accumulated on every map change because `plugin_init` re-ran without clearing prior state. Clearing was unsafe (segfaults) because C++ modules register state during `AMXX_Attach` that must persist. Fixed with dedup-at-registration for all subsystems:
- Commands, SP forwards, multi-forwards, events, log events, messages, menus
- `setCmdType()` changed to return bool, preventing secondary list accumulation
- Result: `plugin_init` flat at ~0.9ms regardless of map changes (was growing to 107ms+)

## [2.6.14] - 2026-03-05

### Fixed

#### Core AMXX - amx_ExecPerf Hot Path Optimization

`amx_ExecPerf()` called `g_plugins.findPluginFast(amx)` on every AMX execution call, even when performance logging was disabled (the default). Moved the plugin lookup inside the `amxmodx_perflog->value > 0.0f` branch so the common path (profiling off) goes straight to `amx_Exec()` with only a single float comparison.

#### Core AMXX - g_putinserver O(n) Removal Replaced with Compact Pattern

`SV_Frame_RH` and `C_StartFrame_Post` both used `g_putinserver.remove(i)` which shifts all remaining elements on every removal — O(n) per removal, O(n²) worst case during peak player joins. Replaced with a single-pass compact pattern: valid entries are written to `writeIdx`, then the vector is resized once at the end. All removals are now O(1).

#### Core AMXX - Extension Mode Missing ClearMenus and FrameAction Cleanup

`KTPAMX_ReloadPlugins()` (extension mode map change) did not call `ClearMenus()` or `g_frameActionMngr.clear()`, causing menu state and frame actions from the previous map to persist. This could lead to stale menu handler references across map changes. Both are now cleared before plugin re-initialization, matching the Metamod mode cleanup path.

#### DODX Module - DODX_OnMsgBegin Missing Bounds Check

`DODX_OnMsgBegin()` indexed into `modMsgs[msg_id]` and `modMsgsEnd[msg_id]` without validating `msg_id` against `MAX_REG_MSGS`. A corrupt or out-of-range message ID could cause an out-of-bounds array read. Added bounds check matching the existing validation in `DODX_OnMessageHandler`.

#### DODX Module - dod_weaponlist Missing wpnID Bounds Check

`dod_weaponlist` native used `wpnID` (params[2]) to index into `weaponlist[]` array without validating it was in range `[0, WEAPONLIST)`. Also added bounds check on `params[1]` before its use as a `weaponlist[]` index. Prevents out-of-bounds reads from plugin calls with invalid weapon IDs.

#### DODX Module - Grenade Tracking Leak on Map Change

`g_grenades` linked list was not cleared in `DODX_OnChangelevel()`. Grenade entries holding edict pointers from the previous map survived the map change, creating stale pointer references. Added `g_grenades.clear()` to the changelevel handler.

#### DODX Module - Team Scores Not Reset on Map Change

`AlliesScore` and `AxisScore` were not reset in `DODX_OnChangelevel()`. Plugins reading `dod_get_team_score()` early on a new map (before the first TeamScore message) would get stale values from the previous map. Both are now zeroed on map change.

#### DODX Module - Buffer Overflow Protection (6 strcpy → strncpy)

Multiple `strcpy` calls in DODX wrote to fixed-size buffers without length validation:
- `CPlayer::Connect()` — `ip[32]` buffer (CMisc.cpp)
- `CPlayer::initModel()` — `modelclass[64]` buffer (CMisc.cpp)
- `register_cwpn` — `name[32]` and `logname[16]` buffers (NBase.cpp)
- `dodx_objective_set_data` — `cap_message[256]`, `model_neutral[256]`, `model_allies[256]`, `model_axis[256]` fields (NCP.cpp)
- `dodx_area_set_data` — `hud_sprite[256]` field (NCP.cpp)

All replaced with `strncpy` + explicit null termination.

#### Core AMXX - Forward String Parameter Double-strlen Eliminated

`CForward::execute()` and `CSPForward::execute()` called `strlen(str)` for `amx_Allot` sizing, then `amx_SetStringOld()` called `strlen()` again internally to copy the string. Replaced `amx_SetStringOld` with an inlined unpacked char-to-cell copy loop using the pre-computed length, eliminating the redundant `strlen` on every string parameter in every forward call.

#### Core AMXX - Task Manager Free-Slot Scan and startFrame Optimizations

Three improvements to `CTaskMngr`:
- **Active count tracking:** Added `m_ActiveCount` to skip `startFrame()` iteration entirely when no tasks are registered (common during warmup/idle periods)
- **Free-slot hint index:** Added `m_FirstFreeHint` so `registerTask()` starts scanning for free slots from the last-known position instead of index 0, reducing O(n) to amortized O(1) for sequential registrations
- **Self-clear tracking:** `startFrame()` now decrements `m_ActiveCount` and updates the free hint when tasks complete and self-clear, keeping the counters accurate without requiring CTask to call back into the manager

#### DODX Module - sendScore Float Precision Fix

`sendScore` was declared as `int` but used as a time comparison against `gpGlobals->time` (float). The `(int)` cast at assignment truncated the target time, causing the score forward to fire with 0–1.25s delay instead of a consistent 0.25s. Changed `sendScore` from `int` to `float` and removed the truncating cast.

#### DODX Module - CHECK_PLAYERRANGE Rejects Index 0

`CHECK_PLAYERRANGE` macro allowed player index 0 (the world entity), which would access `players[0]` — a summary/unused slot, not a real player. Changed the lower bound check from `x < 0` to `x < 1`.

#### DODX Module - loadRank Buffer Overflow Protection

`RankSystem::loadRank()` read player name and unique ID from the rank file into fixed 64-byte buffers without clamping the length read from file. A corrupted rank file with a length field ≥64 would overflow the buffer. Added length clamping to `sizeof(buffer) - 1` and explicit null termination after each read.

## [2.6.13] - 2026-03-04

### Fixed

#### Core AMXX - CTaskMngr Use-After-Free on Changelevel During Task Callback

`CTaskMngr::startFrame()` iterates tasks and calls `executeIfRequired()`, which runs plugin callbacks. If a plugin callback triggers a synchronous changelevel (via `server_cmd` + `server_exec`), `KTPAMX_ReloadPlugins()` calls `g_tasksMngr.clear()` which destroys the `m_Tasks` vector — while the executing task's `CTask::executeIfRequired()` is still on the call stack. When execution returns, `CTask::clear()` at line 148 calls `delete[]` on a dangling `m_pParams` pointer, crashing with `free(): invalid pointer`.

- Added `m_bInStartFrame` / `m_bDeferredClear` flags to `CTaskMngr`
- When `clear()` is called during `startFrame()`, tasks are individually cleared (params freed, marked free) but the vector is NOT destroyed
- `startFrame()` breaks iteration on deferred clear, then destroys the vector after the loop exits safely
- The executing task returns to `executeIfRequired()`, sees `isFree() == true` at line 135, and returns without double-freeing
- Confirmed via gdb analysis of New York 2 core dump (2026-03-04): crash at `CTask.cpp:66` during `startFrame` → `executeIfRequired` → `clear`

#### Core AMXX - New Menu Handler Use-After-Free

In both Metamod and extension mode `ClientCommand` handlers, the `pMenu` pointer was used after `executeForwards()` without re-validation. If a plugin's menu callback destroyed the menu (via `menu_destroy`), `pMenu` became a dangling pointer.

- **MENU_BACK/MENU_MORE paths:** After `executeForwards(pMenu->pageCallback)`, re-validate with `get_menu_by_id(menu)` before calling `pMenu->Display()`
- **Normal item path:** Capture `pMenu->func` into local `menuFunc` before `executeForwards()`, preventing access to potentially freed memory
- Applied to both Metamod (`C_ClientCommand`) and extension mode (`SV_ClientCommand_RH`) code paths

## [2.6.12] - 2026-03-03

### Fixed

#### Core AMXX - Forward Execute Invalid Pointer Crash Prevention

`CForward::execute()` and `CSPForward::execute()` in `CForward.cpp` only checked for NULL string pointers before calling `strlen()`. When a forward parameter type mismatch caused a cell value (e.g., player index `1`) to be reinterpreted as `const char*`, the resulting pointer `0x1` passed the NULL check but crashed on `strlen()`.

- Both execute methods now reject pointers below `0x1000` (first page, always unmapped on Linux)
- Logs a WARNING with forward name, parameter index, function ID, and the bad pointer value for diagnosis
- Defaults to empty string instead of crashing
- STRINGEX cleanup paths also guarded to prevent crash when copying back to an invalid address
- Confirmed via gdb analysis of 2 Atlanta core dumps (2026-03-02): both crashed at `CForward.cpp:282` with `str = 0x1`

#### Core AMXX - SP Forward Free List Reuse Bug

Both `registerSPForward()` overloads in `CForward.cpp` had a bug where the free list entry was popped AFTER an early-return check. When `pForward->Set()` succeeded but `getFuncsNum() == 0`, the function returned `-1` without popping from the free list. The slot was left with `isFree = false` (set by `Set()`) but still queued in `m_FreeSPForwards`, causing the slot to be reused later with potentially stale parameter types.

- Moved `m_FreeSPForwards.pop()` before the early-return check
- On failed registration, properly re-marks the slot as free and pushes it back to the free list

## [2.6.11] - 2026-02-25

### Fixed

#### DODX Module - Missing pvPrivateData Null Checks

`dodx_set_user_class` and `dodx_set_user_team` natives accessed `pEdict->pvPrivateData` without null-checking it first, inconsistent with other KTP natives (grenade, noclip, teamname) that properly validate before access.

- `dodx_set_user_class` — Added `|| !pPlayer->pEdict->pvPrivateData` guard
- `dodx_set_user_team` — Added `|| !pPlayer->pEdict->pvPrivateData` guard

#### DODX Module - CRank IP Lookup Infinite Loop (Stock AMXX Bug)

`CRank.cpp:findEntryInRank()` with IP-based ranking had `a = a->prev` inside the `strncmp` match block only. When `strncmp` didn't match (common case), the loop variable never advanced, causing an infinite loop. Moved `a = a->prev` outside the conditional so the linked list always advances.

This is a stock AMX Mod X bug. Does not affect KTP production (rank system is skipped in extension mode) but fixed for correctness.

#### Core AMXX - SP Forward Dedup Parameter Type Mismatch (Crash Fix)

Both `registerSPForward` overloads in `CForward.cpp` matched on `amx + func/name` only, ignoring `numParams` and `paramTypes`. When the same Pawn function was registered as both a menu callback (FP_CELL params) and a curl/discord callback (FP_STRING params), the dedup returned the wrong forward handle. Integer values (e.g., menu selection `1`) were then cast to `const char*` and passed to `strlen()`, causing a segfault at address `0x1` in `CSPForward::execute`.

- Both overloads now compare `numParams` and `paramTypes` via `memcmp` in addition to `amx + func/name`
- Fixes 4 confirmed production crashes (3x NY, 1x ATL) on 2026-02-27

#### Core AMXX - C_ClientCvarChanged Null Guard

`C_ClientCvarChanged` called `GET_PLAYER_POINTER(pEntity)` without validating `pEntity`. Added null check, `FNullEnt` check, and player index range validation before accessing the player array. Uses `GET_PLAYER_POINTER_I` with validated index instead of raw `GET_PLAYER_POINTER` macro.

#### DODX Module - Debug Ammo Dump Out-of-Bounds Read

`dodx_debug_dump_ammo` scanned pvPrivateData offsets 0–400 (1600 bytes), well beyond the ~700 byte DoD player private data structure. Reduced scan range to 0–175 (700 bytes) to stay within safe bounds.

### Changed

#### Shared Include - ktp_discord.inc v1.3.2

- Fixed duplicate audit messages when `discord_channel_id_admin` matches an audit channel ID — added `_ktp_discord_audit_add()` dedup helper
- Removed `g_ktpDiscordTempFile` dead code (unused since v1.1.0, 128 cells freed per plugin)
- Changed `containi` to `contain` for audit key matching (keys already lowercased)

---

## [2.6.10] - 2026-02-17

### Fixed

#### Extension Mode Subsystem Re-Registration Leak

In extension mode, `KTPAMX_ReloadPlugins()` fires `plugin_init` on every map change without clearing subsystem registrations. Each map change re-registered all commands, forwards, events, log events, messages, and menu commands — causing linear growth in plugin_init time (~2ms/map, reaching 100ms+ after 50 maps).

**Two-pronged fix:**

1. **Module cleanup callback** — Call `modules_callPluginsUnloading()` before `plugin_init` in `KTPAMX_ReloadPlugins()`. This notifies modules (e.g., KTP-ReAPI) to clear plugin-owned state like hookchain vectors before plugins re-register them.

2. **Registration-time deduplication** — All AMXX subsystems now detect duplicate registrations and return existing handles instead of allocating new entries:

| Subsystem | Dedup Key | File |
|-----------|-----------|------|
| Commands (`CmdMngr`) | plugin + command line | `CCmd.cpp` |
| SP Forwards (`CForwardMngr`) | AMX + function index/name | `CForward.cpp` |
| Multi-Forwards (`CForwardMngr`) | function name + exec type + param count | `CForward.cpp` |
| Events (`EventsMngr`) | plugin + function + message ID | `CEvent.cpp` |
| Log Events (`LogEventsMngr`) | plugin + function | `CLogEvent.cpp` |
| Messages (`MessageHooks`) | message ID + function | `messages.h` |
| Menu Commands (`MenuMngr`) | plugin + function + menu keys | `CMenu.cpp` |

3. **`setCmdType()` guard** — Changed return type from `void` to `bool`. Returns `false` if command type bits are unchanged, preventing duplicate entries in secondary command lists and redundant `REG_SVR_COMMAND` engine calls.

**Result:** plugin_init time is now flat at ~0.9ms regardless of map change count (was 107ms+ at 55 map changes — 120x improvement).

**Technical note:** Subsystem clearing (e.g., `g_commands.clear()`) is NOT safe because C++ modules register state once during `AMXX_Attach` — clearing subsystems destroys module state and causes delayed segfaults. Dedup-at-registration is the correct approach.

---

## [2.6.9] - 2026-02-01

### Added

#### DODX Module - Runtime Pdata Offset Detection

Auto-detection of Linux pdata offsets for grenade ammo manipulation:

- **Ubuntu 22.04 and older** - Uses +5 offset adjustment
- **Ubuntu 24.04 and newer** - Uses +4 offset adjustment
- **Auto-detection on first spawn** - Probes memory at both offsets to find valid grenade count
- **No recompilation needed** - Works across Ubuntu versions automatically

**New Globals:**
- `g_iLinuxPdataOffsetAdjust` - Current offset adjustment (4 or 5)
- `g_bPdataOffsetDetected` - Whether detection has run

**New Function:**
- `DODX_DetectPdataOffset(edict_t*)` - Probes player pdata to detect correct offset

**Detection Strategy:**
1. Look for value 1-10 at expected grenade offset
2. Check if same value appears at all 3 redundant offsets (they should match)
3. If +4 matches, use +4; if +5 matches, use +5; otherwise default to +4

**Use Case:** Denver bare-metal runs Ubuntu 24.04, Atlanta runs Ubuntu 22.04. This eliminates the need for separate binaries.

#### DODX Module - New Grenade Natives

- **`dodx_strip_grenade(id, grenade_type)`** - Remove grenade from player and clear ammo slots
  - Clears all 3 ammo slots for the specified grenade type
  - Returns 1 on success, 0 on failure

- **`dodx_debug_dump_ammo(id)`** - Debug utility to dump pdata ammo offsets
  - Scans player pdata for values 1-10 (potential grenade counts)
  - Shows current offset adjustment and expected offsets
  - Useful for debugging offset issues on new OS versions

---

## [2.6.8] - 2026-01-31

### Added

#### Extension Mode Header Stubs
Complete Metamod-free compilation support for third-party modules:

**amxmodx/amxmodx.h:**
- Added Metamod enum stubs (`PLUG_LOADTIME`, `PL_UNLOAD_REASON`)
- Added `hudtextparms_t` struct definition
- Added engine function macros (`INDEXENT`, `VARS`, `IS_DEDICATED_SERVER`, etc.)
- Added cvar macros (`CVAR_GET_POINTER`, `CVAR_REGISTER`, etc.)
- Added info key macros (`GET_INFO_KEY_BUFFER`, `INFO_KEY_VALUE`, etc.)
- Added game DLL function wrapper stubs (`MDLL_Spawn`, etc.)

**public/sdk/amxxmodule.h:**
- Mirror stub definitions for third-party module compilation
- Enables modules like amxxcurl to compile without Metamod SDK headers

**amxmodx/fakemeta.cpp:**
- Added `#ifndef USE_METAMOD` guards for extension mode
- Returns early when Metamod unavailable (no-op instead of crash)

#### Docker Build Support
- `Dockerfile` - Ubuntu 22.04 build environment for glibc 2.35 compatibility
- `docker-build.sh` - Automated Docker build script

### Fixed

- **Module compilation without Metamod** - Third-party modules no longer require Metamod SDK headers when `USE_METAMOD` is not defined

---

## [2.6.7] - 2026-01-24

### Added

#### DODX Module - Pre-Damage Forward
New forward for modifying damage before it's applied:

- **`dod_damage_pre(attacker, victim, damage, wpnindex, hitplace, TA)`** - Fires before `client_damage`
  - Return a lower damage value (0 to damage-1) to reduce damage taken
  - Return 0 to completely block the damage
  - Return original damage (or higher) for no modification
  - **Health message sync** - Automatically sends Health message to victim's HUD after heal-back
  - Stats tracking uses the effective (modified) damage value

**Use Case:** KTPPracticeMode uses this to reduce teammate grenade damage for practice sessions.

#### DODX Module - Give Grenade Native
New native for giving grenades to players (extension mode compatible):

- **`dodx_give_grenade(id, grenade_type)`** - Give a grenade to a player
  - `DODW_HANDGRENADE` (13) - US hand grenade
  - `DODW_STICKGRENADE` (14) - German stick grenade
  - `DODW_MILLS_BOMB` (36) - British Mills bomb
  - Creates weapon entity and touches it to the player
  - Returns 1 on success, 0 on failure

**Use Case:** KTPPracticeMode uses this to give grenades during practice mode.

#### DODX Module - Player Manipulation Natives
New natives ported from dodfun module for extension mode compatibility:

**Class/Team:**
- **`dodx_set_user_class(id, classId)`** - Set player class (1-6, or 0 for random)
- **`dodx_set_user_team(id, teamId, refresh=1)`** - Set player team (1=Allies, 2=Axis, 3=Spectators)
  - Kills player, sets random class
  - `refresh=1` broadcasts team change to all clients

**Position/Angles:**
- **`dodx_get_user_origin(id, Float:origin[3])`** - Get player position
- **`dodx_set_user_origin(id, Float:origin[3])`** - Teleport player
- **`dodx_get_user_angles(id, Float:angles[3])`** - Get player view angles
- **`dodx_set_user_angles(id, Float:angles[3])`** - Set player view angles (includes fixangle)

**Use Case:** These enable player state save/restore during hostname broadcast operations where the server briefly respawns players.

**New Private Data Offsets (CMisc.h):**
- `STEAM_PDOFFSET_CLASS` (367 + Linux offset) - Player class
- `STEAM_PDOFFSET_RCLASS` (368 + Linux offset) - Random class flag

### Fixed

#### Multi-Victim Grenade Damage
- **Freed entity handling** - Grenade entities can be freed after damaging the first victim but before subsequent victims are processed
- **Solution** - Check `enemy->free` flag and fall back to grenade lookup table when entity is freed
- **Result** - Grenade damage now correctly attributes to the thrower for all victims

---

## [2.6.6] - 2026-01-23

### Added

#### DODX Module - AmmoX HUD Sync Native
New native for updating client HUD ammo display after modifying grenade ammo:

- **`dodx_send_ammox(id, ammo_slot, count)`** - Send AmmoX message to update client HUD
  - `ammo_slot=9` for hand grenade / Mills bomb
  - `ammo_slot=11` for stick grenade
  - `count` clamped to 0-254 range
  - Returns 1 on success, 0 on failure

**Use Case:** After calling `dodx_set_grenade_ammo()`, the server-side ammo is updated but the client HUD still shows the old value. Call `dodx_send_ammox()` to sync the client's ammo display.

**Why a native?** AMX Mod X `message_begin()` / `emessage_begin()` crash in extension mode for certain message types. This native uses the engine's `MESSAGE_BEGIN` directly from C++ which works correctly.

**Plugins using this native:**
- KTPGrenadeLoadout - HUD sync after setting spawn grenades
- KTPPracticeMode - HUD sync after refilling grenades

---

## [2.6.5] - 2026-01-23

### Added

#### DODX Module - Noclip Native
New native for player noclip control (ported from fun module for extension mode compatibility):

- **`dodx_set_user_noclip(id, noclip)`** - Set player noclip mode
  - `noclip=0` disables noclip (MOVETYPE_WALK)
  - `noclip=1` enables noclip (MOVETYPE_NOCLIP)
  - Returns 1 on success, 0 on failure

**Use Case:** KTPPracticeMode uses this for the `.noclip` command without requiring the fun module (which needs Metamod).

---

## [2.6.4] - 2026-01-22

### Added

#### DODX Module - Grenade Ammo Natives
New natives for grenade ammo manipulation (extension mode compatible, no Metamod/dodfun required):

- **`dodx_set_grenade_ammo(id, grenade_type, count)`** - Set grenade count for a player
  - Grenade types: `DODW_HANDGRENADE` (13), `DODW_STICKGRENADE` (14), `DODW_MILLS_BOMB` (36)
  - Count clamped to 0-10 range
  - Hand grenade and Mills bomb share the same ammo pool
- **`dodx_get_grenade_ammo(id, grenade_type)`** - Get current grenade count
  - Returns current count or 0 on failure

**New Defines (dodx.h):**
- `PDOFFSET_AMMO_HANDGRENADE_1/2/3` - Private data offsets for hand grenade/Mills bomb ammo
- `PDOFFSET_AMMO_STICKGRENADE_1/2/3` - Private data offsets for stick grenade ammo
- Linux offsets have +5 adjustment per DoD convention

**Use Case:** KTPGrenadeLoadout and KTPPracticeMode plugins use these for grenade customization.

### Fixed

#### SV_CheckConsistencyResponse Hook (Extension Mode)
- **Inverted condition bug** - Hook was triggering forward for CONSISTENT files instead of INCONSISTENT
  - Bug: `if (!result && ...)` fired when `result=false` (file matched)
  - Fix: `if (result && ...)` fires when `result=true` (file mismatched)
- **Return value semantics** - Clarified and fixed return values
  - Hook returns `FALSE` = file OK (allow player)
  - Hook returns `TRUE` = file bad (kick player)
  - Forward returns `1` (PLUGIN_HANDLED) = allow player to stay
- **Model checking now works** - `fc_checkmodels 1` correctly kicks players with modified models

#### force_unmodified() Timing (Extension Mode)
- **Root cause** - `ENGINE_FORCE_UNMODIFIED` only works during spawn/precache phase
- **Solution** - Added `PF_precache_model_I` hook to initialize AMXX during precache
  - Full AMXX init (modules, plugins, hooks) runs on first precache call
  - `plugin_precache` forward executes so plugins can call `force_unmodified()`
  - Force lists processed with `ENGINE_FORCE_UNMODIFIED` while still in precache phase
  - `plugin_init`/`plugin_cfg` deferred to `SV_ActivateServer` (game state not ready during precache)
- **Map change fix** - Reset `g_bExtPrecacheProcessed` and clear force lists in `SV_InactivateClients`
  - Without this, `plugin_precache` wouldn't fire on subsequent maps

### Technical

**New Global Variables:**
- `g_bExtPrecacheProcessed` - Tracks if precache hooks processed force lists this map
- `g_bInitDuringPrecache` - Tracks if AMXX init was called during precache phase

**New Hook:**
- `PF_precache_model_I` - Fires during engine precache, triggers early AMXX initialization

---

## [2.6.3] - 2026-01-06

### Added

#### ktp_discord.inc v1.2.0 - Draft Channel Support

- **`KTP_DISCORD_CHANNEL_DRAFT`** - New channel type constant (value 5) for draft match Discord posts
- **`discord_channel_id_draft`** - New config key in discord.ini for draft channel ID
- **`g_ktpDiscordChannelDraft`** - Storage variable for draft channel
- **Draft channel getter** - `ktp_discord_get_channel(KTP_DISCORD_CHANNEL_DRAFT, ...)` returns draft channel (no fallback)

---

## [2.6.2] - 2025-12-31

### Added

#### DODX Module - New Natives for Score Broadcasting

Two new natives for scoreboard manipulation:

- **`dodx_broadcast_team_score(team, score)`** - Broadcast TeamScore message to all clients
  - Sets gamerules score AND sends TeamScore message in one operation
  - Avoids server crashes that occurred with AMX message natives
  - Used by KTPMatchHandler for 2nd half score restoration
  - Returns 1 on success, 0 on failure

- **`dodx_set_scoreboard_team_name(team, const name[])`** - Set custom team name on scoreboard
  - Sends TeamInfo message to all clients for players on specified team
  - May override hardcoded "Allies"/"Axis" display on client scoreboard
  - Returns number of players updated

#### ktp_discord.inc Cleanup
- Removed unused `g_ktpDiscordConfigLoaded` variable

---

## [2.6.1] - 2025-12-26

### Changed

#### ktp_discord.inc v1.1.0
Major rewrite of Discord integration include:

- **AMXX curl module** - Switched from `server_cmd("curl...")` to proper AMXX curl module
  - `server_cmd()` cannot execute shell commands on Linux servers
  - Now uses `curl_easy_init()`, `curl_easy_perform()` with async callbacks
- **Fixed JSON field names** - Match Discord Relay API expectations
  - `channel_id` → `channelId`
  - `payload.embeds` → `embeds` (top level)
- **Added curl cleanup** - Proper `curl_easy_cleanup()` and `curl_slist_free_all()` in callback
- **Debug logging** - Added `log_amx()` calls for troubleshooting Discord issues

#### reapi_engine_const.inc
- **RH_SV_Rcon hook** - Added enum constant for new KTP-ReHLDS RCON audit hook
  - Parameters: `(const command[], const from_ip[], bool:is_valid)`
  - Used by KTPAdminAudit v2.2.0 for RCON command logging

---

## [2.6.0] - 2025-12-21

### Added

#### New Native: ktp_drop_client
New native for dropping clients via ReHLDS API, bypassing blocked kick console command:

**New Native:**
- **`ktp_drop_client(id, const reason[] = "")`** - Drop client using ReHLDS DropClient API
  - Bypasses blocked kick command in KTP ReHLDS
  - Works in extension mode (no Metamod required)
  - Requires ReHLDS API to be available
  - Returns 1 on success, 0 on failure

**Use Case:**
- KTPAdminAudit uses this to execute kicks after menu-based admin approval
- Allows kick functionality when console `kick` command is blocked at engine level

#### New Include: ktp_discord.inc
Shared Discord integration include for KTP plugins:

**Features:**
- Common Discord configuration loading from `discord.ini`
- Audit channel ID retrieval
- Shared webhook/relay integration pattern

**Plugins using this include:**
- KTPAdminAudit
- KTPCvarChecker
- KTPFileChecker
- KTPMatchHandler

### Technical Details
- Native implemented in `amxmodx.cpp`
- Uses `g_RehldsApi->GetFuncs()->DropClient()` for direct client drop
- Include file location: `plugins/include/ktp_discord.inc`

---

## [2.5.1] - 2025-12-20

### Added

#### DODX Module - Player Team Name Native
New native for setting player team names in private data (extension mode compatible):

**New Native:**
- **`dodx_set_pl_teamname(id, szName[])`** - Set player's team name in private data
  - Affects server-side logging (team name in kill logs, etc.)
  - Works in extension mode (no Metamod required)
  - Max 15 characters + null terminator
  - Note: Does NOT affect scoreboard (DoD client hardcodes "Allies"/"Axis")

**New Defines (dodx.h):**
- `STEAM_PDOFFSET_TEAMNAME` - Player private data offset for team name (1400 Windows, 1405 Linux)
- `STEAM_PDOFFSET_SCORE` - Player score offset
- `STEAM_PDOFFSET_DEATHS` - Player deaths offset

**New Message Registration:**
- `gmsgTeamInfo` - For potential future scoreboard refresh functionality

### Technical Details
- Native implemented in `NBase.cpp`
- Uses same offsets as dodfun module for compatibility
- 16-byte null-padded copy to match engine expectations

---

## [2.5.0] - 2025-12-19

### Added

#### HLStatsX Integration
New DODX natives for match-based statistics tracking with KTPMatchHandler:

**New Natives:**
- **`dodx_flush_all_stats()`** - Fire `dod_stats_flush` forward for all connected players
  - Allows flushing warmup stats before match starts
  - Returns number of players flushed
- **`dodx_reset_all_stats()`** - Reset all accumulated stats for all players
  - Clears weapons[], attackers[], victims[], weaponsLife[], weaponsRnd[], life, round
  - Call after flushing to start fresh for match
- **`dodx_set_match_id(matchId[])`** - Set match ID for stats correlation
  - When set, weaponstats log lines include `(matchid "xxx")` property
  - Pass empty string to clear match context
- **`dodx_get_match_id(output[], maxlen)`** - Get current match ID

**New Forward:**
- **`dod_stats_flush(id)`** - Called by `dodx_flush_all_stats()` for each player
  - stats_logging.sma registers for this to log pending weaponstats

#### stats_logging.sma Updates
- **Match ID support** - All log lines include `(matchid "xxx")` when match ID is set
- **`dod_stats_flush` handler** - Logs weaponstats on demand (for warmup flush)
- **`log_player_stats()` stock** - Refactored from client_disconnected for reuse

### Technical Details

**Intended workflow for KTPMatchHandler:**
1. During warmup: stats accumulate normally
2. Match start: `dodx_flush_all_stats()` → logs warmup stats without match ID
3. Match start: `dodx_reset_all_stats()` → clears stats for fresh match
4. Match start: `dodx_set_match_id("KTP-1234567890-dod_charlie")` → sets context
5. During match: stats accumulate with match ID
6. Match end: Stats logged on disconnect include match ID
7. Match end: `dodx_set_match_id("")` → clears context for warmup

---

## [2.4.0] - 2025-12-16

### Added

#### DODX Extension Mode - Complete Rewrite
The DODX module has been extensively rewritten for full extension mode support:

**New ReHLDS Hook Handlers:**
- `DODX_OnPlayerPreThink` - Main stats tracking loop (replaces `FN_PlayerPreThink_Post`)
- `DODX_OnClientConnected` - Player connection handling (replaces `FN_ClientConnect_Post`)
- `DODX_OnSV_Spawn_f` - Player spawn handling (replaces `FN_ClientPutInServer_Post`)
- `DODX_OnSV_DropClient` - Player disconnect handling (replaces `FN_ClientDisconnect`)
- `DODX_OnChangelevel` - Pre-changelevel cleanup to prevent stale pointer crashes
- `DODX_OnTraceLine` - Hit detection and aiming (replaces `TraceLine_Post`)
- `IMessageManager` hooks for 16 game message types

**Shot Tracking via Button State:**
- Tracks weapon shots via IN_ATTACK button monitoring in PreThink
- Detects rising edge (new shots) and held attack (automatic weapons)
- Per-weapon fire rate delays for accurate shot counting:
  - MG42: 0.05s | .30 cal, MG34, Bren: 0.08s
  - SMGs (Thompson, MP40, MP44, Sten): 0.1s
  - Semi-auto/bolt rifles: 0.5s (rising edge only)
  - Pistols: 0.3s (rising edge only)
- New CPlayer fields: `oldbuttons`, `lastShotTime`, `nextShotTime`

#### ENTINDEX_SAFE Implementation
- **New inline function** `ENTINDEX_SAFE(edict_t*)` uses pointer arithmetic instead of engine calls
- **New global** `g_pFirstEdict` cached in `ServerActivate_Post` for safe entity index calculation
- **Prevents crashes** from calling engine functions during ReHLDS hooks
- `GET_PLAYER_POINTER` macro updated to use `ENTINDEX_SAFE`

#### Server Active Flag
- **New global** `g_bServerActive` tracks whether server is in valid state for processing
- Set to `true` in `ServerActivate_Post`, `false` in `ServerDeactivate` and `OnChangelevel`
- Prevents message hooks from using stale pointers during map changes

#### Module SDK Extensions
New functions for modules to access engine resources in extension mode:
- **`MF_GetEngineFuncs()`** - Returns pointer to engine function table
- **`MF_GetGlobalVars()`** - Returns pointer to gpGlobals
- **`MF_GetUserMsgId(name)`** - Look up message ID by name (works in extension mode)
- **`MF_RegModuleMsgHandler()`** - Register module message handler callbacks
- **`MF_UnregModuleMsgHandler()`** - Unregister module message handler callbacks
- **`MF_RegModuleMsgBeginHandler()`** - Register message begin handler

#### DODX Deferred Initialization
- Cvar registration moved from `OnAmxxAttach` to `OnPluginsLoaded` (engine not ready earlier)
- Message ID lookup via `MF_GetUserMsgId` instead of engine calls
- Player initialization via PreThink hook (lazy initialization on first frame)

### Fixed

#### Stats Native Safety Hardening
All DODX stats natives now have comprehensive safety checks:
- `gpGlobals` NULL check (can be NULL during map change)
- Player index range validation
- `pEdict` and `pEdict->free` checks before access
- `pPlayer->rank` NULL checks (rank system not used in extension mode)

**Hardened natives:**
- `get_user_astats`, `get_user_vstats`
- `get_user_wstats`, `get_user_wlstats`, `get_user_wrstats`
- `get_user_stats`, `get_user_lstats`, `get_user_rstats`
- `reset_user_wstats`

#### CHECK_PLAYER Macro Rewrite
- Now uses `players[]` array directly instead of `MF_IsPlayerIngame`/`MF_GetPlayerEdict`
- Checks `pEdict->free` before calling `FNullEnt()`
- Prevents crashes when player edict is freed during disconnect

#### TraceLine Hook Safety
- Added `g_bServerActive` and `g_pFirstEdict` checks
- Added `ptr` NULL validation
- Added `pEdict->free` checks for all edict accesses
- Uses `ENTINDEX_SAFE` for all index calculations

#### ServerDeactivate Safety
- Clears `g_bServerActive` and `g_pFirstEdict` at start of function
- Added `gpGlobals` NULL check
- Added `maxClients` range validation with fallback

#### Log File Handling
- Removed `log on` call from stats_logging.sma that caused log rotation
- Logging should be enabled via `sv_logfile 1` in server.cfg only

### Changed

#### Debug Logging Cleanup
- Removed all `[DODX DEBUG]` and `[KTPAMX DEBUG]` statements
- Removed debug counters and tracking variables
- Cleaned up verbose initialization logging

#### Startup Message Cleanup
Removed verbose messages, kept only essential operational output:
- Kept: `[KTP AMX] ReHLDS extension mode detected...`
- Kept: `[DODX] Running in ReHLDS extension mode.`
- Kept: `KTP AMX initialized as ReHLDS extension (no Metamod)`
- Kept: `[KTP AMX] Loaded X plugin(s).`

---

## [2.3.0] - 2025-12-14

### Added

#### DODX Extension Mode Fully Functional
- **PF_TraceLine hook** - Hit detection and aiming statistics now work in extension mode
  - POST hook only - reads trace results without affecting gameplay
  - Safe for wallpen (doesn't interfere with wallbang detection)
- **All 4 DODX hooks now active** in extension mode:
  - `SV_PlayerRunPreThink` - Stats tracking loop
  - `PF_changelevel_I` - Pre-changelevel cleanup
  - `PF_TraceLine` - Hit detection/aiming
  - `IMessageManager` - 16 message hooks for game stats

### Fixed

#### stats_logging.sma Disconnect Crash
- **Root cause**: `get_user_wstats` called during `client_disconnected` crashed because the player's edict was already marked as free
- **Solution**: Hardened `CHECK_PLAYER` macro in `dodx.h` to check `edict->free` before calling `FNullEnt()`
- **Result**: stats_logging.sma now works correctly for end-of-round logging

#### stats_logging.sma Verified Working
- **Tested and confirmed**: Plugin logs `weaponstats`, `weaponstats2`, `time`, and `latency` on disconnect
- **Log output**: Correctly written to HLDS log files for stats parsers
- **Startup fix**: Added `set_task(1.0, "enable_logging")` to force `log on` after server startup

#### DODX Safety Hardening
- **ENTINDEX_SAFE conversion** - All raw `ENTINDEX()` calls converted to `ENTINDEX_SAFE()` using pointer arithmetic
- **pEdict access hardening** - All pEdict accesses now have `if (!pEdict || pEdict->free)` guards
- **Prevents crashes** from stale or invalid edict pointers during map changes and player disconnects

### Technical Details

#### CHECK_PLAYER Macro Fix (dodx.h)
Before:
```cpp
if (!MF_IsPlayerIngame(x) || FNullEnt(MF_GetPlayerEdict(x)))
```
After:
```cpp
edict_t* _pEdict = MF_GetPlayerEdict(x);
if (!MF_IsPlayerIngame(x) || !_pEdict || _pEdict->free || FNullEnt(_pEdict))
```

#### ENTINDEX_SAFE Implementation (dodx.h)
```cpp
inline int ENTINDEX_SAFE(const edict_t *pEdict) {
    if (!pEdict || !g_pFirstEdict)
        return 0;
    return static_cast<int>(pEdict - g_pFirstEdict);
}
```

---

## [2.2.0] - 2025-12-08

### Added

#### Extension Mode Event Support
- **`register_event` in extension mode** - Events now work via KTPReHLDS IMessageManager integration
  - `MessageHook_Handler` parses message parameters and fires AMXX event callbacks
  - Hooks installed on-demand when plugins call `register_event`
- **`register_logevent` in extension mode** - Log events work via AlertMessage hookchain
  - Filters `at_logged` messages and fires log event handlers
  - Also triggers `plugin_log` forward

#### Module API for Extension Mode
- `MF_IsExtensionMode()` - Check if running without Metamod
- `MF_GetRehldsApi()` - Access ReHLDS API from modules
- `MF_GetRehldsHookchains()` - Access ReHLDS hookchains
- `MF_GetRehldsFuncs()` - Access ReHLDS functions
- `MF_GetRehldsServerData()` - Access ReHLDS server data
- `MF_GetRehldsMessageManager()` - Access IMessageManager
- `MF_GetGameDllFuncs()` - Access game DLL functions

#### Module Compatibility Testing
- **amxxcurl**: Confirmed working in extension mode (uses `MF_RegModuleFrameFunc`)
- **ReAPI**: Confirmed working in extension mode (has dedicated extension support)
- **DODX**: Extension hooks added but disabled due to crashes (`#if 0`)
- **SQLite**: Crashes in extension mode (Metamod hooks incompatible)

### Changed
- Default module suffix changed from `_amxx` to `_ktp`
- Module loader now recognizes both `_amxx` and `_ktp` suffixes

### Fixed
- `getEventId()` now works in extension mode using `REG_USER_MSG` lookup

### Technical Details

#### IMessageManager Integration
KTPReHLDS 3.16+ provides `IMessageManager` for intercepting network messages without Metamod. KTPAMXX now:
1. Calls `RehldsMessageManager->registerHook(msg_id, handler)` per message type
2. `MessageHook_Handler` parses `IMessage` parameters (byte, short, long, float, string)
3. Calls `g_events.parseValue()` and executes registered event handlers

#### AlertMessage Hookchain
New `AlertMessage` hookchain in KTPReHLDS provides pre-formatted log strings:
1. Hook fires with `ALERT_TYPE` and formatted message
2. KTPAMXX filters for `at_logged` type
3. Passes to `g_logevents` for parsing and execution

---

## [2.1.0] - 2025-12-06

### Added

#### New ReHLDS Hooks for Extension Mode
- **SV_ClientCommand hookchain** - Enables `register_clcmd`, menu systems, and `client_command` forward in extension mode
- **SV_InactivateClients hookchain** - Proper map change deactivation with `plugin_end` and `client_disconnect` forwards
- **SV_Spawn_f hookchain** - Client reinitialization after map change for `client_connect` and `client_putinserver` forwards

#### Map Change Support (Extension Mode)
- Clients now persist through map changes without disconnection
- All AMXX forwards (`plugin_init`, `plugin_cfg`, `client_connect`, `client_putinserver`) fire correctly on new maps
- Proper `plugin_end` and `client_disconnect` forwards during map transition

#### Client Command Processing (Extension Mode)
- Chat commands (`/start`, `.start`, etc.) now work in extension mode
- Menu selections (`menuselect 1-9`) properly handled
- `register_clcmd` and `register_menucmd` fully functional

### Fixed

- **SV_Spawn_f hook registration** - Function existed but was never registered, causing map change reconnect issues
- **Vtable alignment** - Fixed mismatch between KTPAMXX and KTPReHLDS headers (added 20+ missing virtual methods to IRehldsHookchains)
- **Debug logging cleanup** - Removed all debug `AMXXLOG_Log` statements from production code

### Changed

- **rehlds_api.h** - Updated to match KTPReHLDS vtable layout with all hookchain methods
- **mod_rehlds_api.cpp** - Updated for new API structure
- Removed deprecated debug logging from CForward.cpp, CMisc.cpp, CTask.cpp, amxmodx.cpp, cvars.cpp, meta_api.cpp

### Technical Details

#### New Hook Registrations (Extension Mode)
- `SV_ClientCommand` - Client command processing for chat commands and menus
- `SV_InactivateClients` - Map change deactivation sequence
- `SV_Spawn_f` - Client spawn command after map change reconnect

#### Map Change Sequence
The extension mode now properly handles the map change sequence:
1. `SV_InactivateClients()` → Fire disconnect forwards, clear player state
2. `SV_ActivateServer()` → Fire `plugin_init`, `plugin_cfg`
3. `SV_Spawn_f()` → Reinitialize reconnecting clients, fire `client_connect`, `client_putinserver`

---

## [2.0.0] - 2025-12-04

### Added

#### ReHLDS Extension Mode (Metamod-Free Operation)
- **Standalone ReHLDS extension support** - KTP AMX can now run as a direct ReHLDS extension without requiring Metamod
- **Dual-mode operation** - Automatically detects and adapts to both Metamod and extension modes
- **ReHLDS API integration** - Full integration with ReHLDS hooks and callbacks
- **Game DLL function wrappers** (`KTPAMX_*` macros) - Unified API that works in both operating modes
- **Module frame callbacks** - `MNF_RegModuleFrameFunc()` / `MNF_UnregModuleFrameFunc()` for modules requiring per-frame processing (replaces Metamod's pfnStartFrame for modules)

#### New Forward: `client_cvar_changed`
- **Real-time cvar monitoring** - Instant notification when clients respond to ANY cvar query
- **Event-driven architecture** - Zero polling overhead, 100% callback-based
- **KTP-ReHLDS integration** - Receives `pfnClientCvarChanged` callbacks from modified engine
- **Simple plugin API** - Single forward handler receives all cvar responses

#### Build System Improvements
- **Windows build support** - `build_windows.bat` for native Windows builds
- **Linux build support** - `build_linux.sh` with WSL compatibility
- **Cross-platform packaging** - `collect_builds.bat` to gather builds from both platforms
- **WSL build integration** - `build_linux_wsl.ps1` and `setup_wsl_build.ps1` for building Linux binaries from Windows
- **Plugin compilation toggle** - `--disable-plugins` configure flag to skip plugin compilation

#### Binary Renaming (KTP Branding)
- Main binary renamed from `amxmodx_mm` to `ktpamx` (Windows: `ktpamx.dll`, Linux: `ktpamx_i386.so`)
- Module binaries renamed from `*_amxx` suffix to `*_ktp` suffix
- Updated all default paths from `addons/amxmodx/` to `addons/ktpamx/`

### Changed

#### Core Architecture
- **Hybrid initialization** - Supports both `Meta_Attach()` (Metamod) and `AMXX_RehldsExtensionInit()` (ReHLDS extension)
- **Game DLL interface abstraction** - `g_pGameEntityInterface` pointer works in both modes
- **Forward execution** - All standard forwards now work in extension mode
- **Server command handling** - `amx` prefix commands work without Metamod

#### Path Defaults Updated
- `amxx_configsdir` default: `addons/ktpamx/configs`
- `amxx_pluginsdir` default: `addons/ktpamx/plugins`
- `amxx_datadir` default: `addons/ktpamx/data`

#### Build System
- `AMBuildScript` - Updated for KTP binary naming and optional plugin compilation
- `PackageScript` - Reorganized packaging for KTP AMX distribution
- `configure.py` - Added `--disable-plugins` option

#### Code Quality
- Added extensive debug logging throughout core systems (controlled by `AMXXLOG_Log`)
- Improved CRLF handling in config file parsing (CVault.cpp)
- Fixed cvar registration to properly handle pre-existing config values
- Added null pointer checks throughout player handling code

### Fixed

- **CVault CRLF handling** - Fixed "Can't use values with ASCII control characters" errors when config files have Windows line endings
- **Cvar registration race condition** - Properly handle cvars set by configs before plugin registration
- **Extension mode game DLL access** - Properly resolve server library base address in non-Metamod mode
- **Module loading in extension mode** - Graceful handling of Metamod-dependent modules

### Technical Details

#### New Global Variables
- `g_bRunningWithMetamod` - Boolean flag indicating Metamod presence
- `g_bRehldsExtensionInit` - Boolean flag indicating ReHLDS extension initialization
- `g_pGameEntityInterface` - Pointer to game DLL functions (works in both modes)

#### New Exported Functions (Extension Mode)
- `AMXX_RehldsExtensionInit()` - Entry point for ReHLDS extension loading
- `AMXX_RehldsExtensionShutdown()` - Cleanup for extension unloading

#### Hook Registrations (Extension Mode)
- `SV_DropClient` - Client disconnect handling
- `SV_ActivateServer` - Server activation (map load)
- `Cvar_DirectSet` - Cvar change monitoring
- `SV_WriteFullClientUpdate` - Client info updates
- `ED_Alloc` / `ED_Free` - Entity allocation tracking
- `SV_StartSound` - Sound emission hook
- `PF_Remove_I` - Entity removal hook
- `ClientConnected` / `SV_ConnectClient` - Client connection handling

### Compatibility Notes

- **Requires KTP-ReHLDS** for `client_cvar_changed` forward functionality
- **Backwards compatible** - All existing AMX Mod X plugins work unchanged
- **Standard ReHLDS compatible** - Extension mode works with standard ReHLDS (without `client_cvar_changed`)
- **Metamod compatible** - Can still run as traditional Metamod plugin

## [1.10.0] - Upstream

Base version forked from AMX Mod X 1.10.0.5468-dev.

See [AMX Mod X releases](https://github.com/alliedmodders/amxmodx/releases) for upstream changelog.

---

## Version History Summary

| Version | Date | Description |
|---------|------|-------------|
| 2.6.13 | 2026-03-04 | CTaskMngr use-after-free on changelevel during task callback, new menu handler use-after-free |
| 2.6.12 | 2026-03-03 | Forward execute invalid pointer crash prevention, SP forward free list reuse bug fix |
| 2.6.11 | 2026-02-25 | SP forward dedup crash fix, pvPrivateData null checks, CRank infinite loop fix, cvar null guard |
| 2.6.10 | 2026-02-17 | Extension mode subsystem dedup: flat plugin_init time across map changes |
| 2.6.9 | 2026-02-01 | DODX runtime pdata offset detection for Ubuntu 22.04/24.04 |
| 2.6.8 | 2026-01-31 | Extension mode header stubs, Docker build support |
| 2.6.7 | 2026-01-24 | DODX dod_damage_pre forward, dodx_give_grenade + player manipulation natives, grenade fix |
| 2.6.6 | 2026-01-23 | DODX dodx_send_ammox native for HUD ammo sync |
| 2.6.5 | 2026-01-23 | DODX dodx_set_user_noclip native |
| 2.6.4 | 2026-01-22 | DODX grenade ammo natives, consistency hook fix, precache timing fix |
| 2.6.3 | 2026-01-06 | ktp_discord.inc v1.2.0: Draft channel support |
| 2.6.2 | 2025-12-31 | DODX score broadcasting natives, ktp_discord.inc cleanup |
| 2.6.1 | 2025-12-26 | ktp_discord.inc v1.1.0 (curl module), RH_SV_Rcon hook constant |
| 2.6.0 | 2025-12-21 | ktp_drop_client native, ktp_discord.inc shared include |
| 2.5.1 | 2025-12-20 | DODX dodx_set_pl_teamname native for player team names |
| 2.5.0 | 2025-12-19 | HLStatsX integration: match ID, stats flush/reset natives |
| 2.4.0 | 2025-12-16 | DODX shot tracking, module SDK extensions, log file fix, debug cleanup |
| 2.3.0 | 2025-12-14 | DODX extension mode complete, TraceLine hook, stats_logging crash fix |
| 2.2.0 | 2025-12-08 | register_event/register_logevent extension mode, module API |
| 2.1.0 | 2025-12-06 | Map change support, client commands, menu systems in extension mode |
| 2.0.0 | 2025-12-04 | Major release: ReHLDS extension mode, KTP branding, client_cvar_changed |
| 1.10.0 | - | Base fork from AMX Mod X |

[2.6.13]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.13
[2.6.12]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.12
[2.6.11]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.11
[2.6.10]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.10
[2.6.9]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.9
[2.6.8]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.8
[2.6.7]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.7
[2.6.6]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.6
[2.6.5]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.5
[2.6.4]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.4
[2.6.3]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.3
[2.6.2]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.2
[2.6.1]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.1
[2.6.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.6.0
[2.5.1]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.5.1
[2.5.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.5.0
[2.4.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.4.0
[2.3.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.3.0
[2.2.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.2.0
[2.1.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.1.0
[2.0.0]: https://github.com/afraznein/KTPAMXX/releases/tag/v2.0.0
[1.10.0]: https://github.com/alliedmodders/amxmodx/releases/tag/1.10.0
