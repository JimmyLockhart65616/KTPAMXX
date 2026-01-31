# Changelog

All notable changes to KTP AMX will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
