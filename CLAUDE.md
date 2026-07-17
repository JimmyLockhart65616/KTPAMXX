# KTPAMXX - Claude Code Context

> **IPs here are placeholders** — this repo is public. Real addresses resolve in
> the private root context (`KTP Git Projects/CLAUDE.md` § IP Addresses),
> which is deliberately not in any git repository.

**REQUIRED: Before modifying any C++ or include files in this repo, invoke the `cpp-dev` skill** (`.claude/skills/cpp-dev/SKILL.md`). It carries the fork-delta rules, the extension-mode init-parity trap, and the build/verify workflow; do not edit source without it loaded.

## Build Command
To build this project, use:
```bash
wsl bash -c "cd '/mnt/n/Nein_/KTP Git Projects/KTPAMXX' && bash build_linux.sh"
```

This will:
1. Build KTPAMXX using AMBuild (Python-based build system)
2. Output to `obj-linux/packages/`
3. Auto-stage to `N:\Nein_\KTP Git Projects\KTP DoD Server\serverfiles\dod\addons\ktpamx\`

## Project Structure
- `build_linux.sh` - Main WSL build script
- `configure.py` - AMBuild configuration
- `obj-linux/` - Build output directory
- `plugins/include/` - AMX include files (used by plugin compiler)
- `obj-linux/packages/base/addons/ktpamx/scripting/amxxpc` - Linux plugin compiler

## Build Output
| File | Destination |
|------|-------------|
| `ktpamx_i386.so` | Main AMXX binary |
| `dodx_ktp_i386.so` | DoD stats module |

## Purpose
Custom AMX Mod X fork that operates in extension mode (loaded directly by KTP-ReHLDS without Metamod). This is the scripting platform layer that provides the plugin compiler and runtime.

## Key Include Files
| Include | Purpose |
|---------|---------|
| `dodx.inc` | DODX natives including score/stats functions |
| `ktp_discord.inc` | Shared Discord integration (curl-based) |
| `reapi_engine_const.inc` | KTP-ReHLDS hook constants |

## DODX Natives (KTP Additions)

### Score Management
| Native | Purpose |
|--------|---------|
| `dodx_has_gamerules()` | Check if gamerules pointer is valid |
| `dodx_set_team_score(team, score)` | Set team score in gamerules |
| `dodx_get_team_score(team)` | Get team score from gamerules |
| `dodx_broadcast_team_score(team, score)` | Broadcast TeamScore message to clients |
| `dodx_set_scoreboard_team_name(team, name)` | Set custom team name on scoreboard |

### Round Timer (v2.7.23+)
| Native | Purpose |
|--------|---------|
| `Float:dodx_get_round_time()` | Engine-authoritative seconds remaining in the current half (reads CDoDTeamPlay gamerules; projects through the clan-restart countdown; `-1.0` fail-soft). For the KTPHudObserver closed-loop broadcast clock. |

### HLStatsX Integration
| Native | Purpose |
|--------|---------|
| `dodx_set_match_id(id)` | Set match ID for stats correlation |
| `dodx_get_match_id(output[], maxlen)` | Get current match ID |
| `dodx_flush_all_stats()` | Fire dod_stats_flush for all players |
| `dodx_reset_all_stats()` | Clear all accumulated stats |

### Player Data
| Native | Purpose |
|--------|---------|
| `dodx_set_pl_teamname(id, szName[])` | Set player's team name in private data |

### Grenade Manipulation (v2.6.4+)
| Native | Purpose |
|--------|---------|
| `dodx_set_grenade_ammo(id, type, count)` | Set grenade count (0-10) for player |
| `dodx_get_grenade_ammo(id, type)` | Get current grenade count |
| `dodx_send_ammox(id, slot, count)` | Send AmmoX message to sync client HUD |
| `dodx_give_grenade(id, type)` | Give a grenade weapon to player |

### Player Manipulation (v2.6.5+)
| Native | Purpose |
|--------|---------|
| `dodx_set_user_noclip(id, noclip)` | Set player noclip mode (0/1) |
| `dodx_set_user_class(id, classId)` | Set player class (1-6 or 0=random) |
| `dodx_set_user_team(id, teamId, refresh)` | Set player team (1=Allies, 2=Axis, 3=Spec) |
| `dodx_get_user_origin(id, Float:origin[3])` | Get player position |
| `dodx_set_user_origin(id, Float:origin[3])` | Teleport player |
| `dodx_get_user_angles(id, Float:angles[3])` | Get player view angles |
| `dodx_set_user_angles(id, Float:angles[3])` | Set player view angles |

### Damage Forward (v2.6.7)
| Forward | Purpose |
|---------|---------|
| `dod_damage_pre(att, vic, dmg, wpn, hit, TA)` | Pre-damage hook, return modified damage |

### Test Dispatch Natives (extension-mode forward drivers)
Synthetic dispatchers for AC/integration tests — fire a forward directly (no fakemeta).
| Native | Purpose |
|--------|---------|
| `dodx_test_dispatch_weapon_fire(id, weapon, Float:gametime)` | Exercises the `dod_client_weapon_fire` forward (mirrors `dodx_test_dispatch_damage`) |
| `dodx_test_dispatch_damage(att, vic, dmg, wpn, hit, TA)` | Exercises `client_damage` |
| `dodx_test_dispatch_grenade_explosion(id, Float:pos[3], wpnid)` | Exercises `dod_grenade_explosion` |
| `dodx_test_dispatch_score(id, delta, total, cp_index)` | Exercises `client_score` + `dod_score_event` (tandem, matches production) |
| `dodx_test_dispatch_cp_captured(cp_index, new_owner, old_owner)` | Exercises `dod_control_point_captured` |
| `dodx_test_dispatch_client_spawn(id)` | Exercises `dod_client_spawn` (added `127f39fc`, 2026-07-04) |
| `dodx_test_dispatch_changeteam(id, team, oldteam)` | Exercises `dod_client_changeteam` |
| `dodx_test_dispatch_changeclass(id, class, oldclass)` | Exercises `dod_client_changeclass` |
| `dodx_test_dispatch_client_death(killer, victim, wpn, hit, TK)` | Exercises `client_death` (killer-first, production order) |
| `dodx_test_dispatch_stats_flush(id)` | Exercises `dod_stats_flush` for one slot (production loops connected players) |

> **Staging note:** 2.7.19 BUILT 2026-07-03 (core async CLog writer + the 2.7.18 dodx natives) and staged to the KTP DoD Server tree — smoke-tested, uncommitted, fleet deploy gated on ReHLDS .927 clearing its canary. See CHANGELOG 2.7.19.

## Dependencies
- KTPhlsdk (HLSDK headers)
- Python 3 with AMBuild
- GCC with 32-bit support

## Server Deployment

Deploy compiled binaries to production servers using Python/Paramiko.

**Server Credentials:** the dodserver SSH password was **rotated 2026-05-31** — the
prior `ktp` is dead. Do NOT hardcode it in scripts (this repo is PUBLIC). See the
main `N:\Nein_\KTP Git Projects\CLAUDE.md` § Server Credentials for the current value.

| Server | Host | User | Password |
|--------|------|------|----------|
| Atlanta | <ATL_BM_GAME_IP> | dodserver | (rotated — see main CLAUDE.md) |
| Dallas | <DAL_GAME_IP> | dodserver | (rotated — see main CLAUDE.md) |
| Denver | <DEN_GAME_IP> | dodserver | (rotated — see main CLAUDE.md) |
| New York | <NYC_GAME_IP> | dodserver | (rotated — see main CLAUDE.md) |

**Remote Paths:**
- `~/dod-{port}/serverfiles/dod/addons/ktpamx/dlls/ktpamx_i386.so`
- `~/dod-{port}/serverfiles/dod/addons/ktpamx/modules/*.so`

See `N:\Nein_\KTP Git Projects\CLAUDE.md` for paramiko SSH documentation.

## Related Projects
- `N:\Nein_\KTP Git Projects\KTPhlsdk` - SDK headers
- `N:\Nein_\KTP Git Projects\KTP DoD Server` - Test server with staged binaries

## Notes
- The plugin compiler (`amxxpc`) from this build is used by all AMX plugin projects
- Include files at `plugins/include/` are the authoritative source for plugin compilation
