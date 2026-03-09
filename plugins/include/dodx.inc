// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

//
// DODX Functions
//

#if defined _dodx_included
	#endinput
#endif
#define _dodx_included

#include <dodconst>
#include <dodstats>

#pragma reqclass xstats
#if !defined AMXMODX_NOAUTOLOAD
	#pragma defclasslib xstats dodx
#endif

/************* Shared Natives Start ********************************/

/* Forward types */
enum {
  XMF_DAMAGE = 0,
  XMF_DEATH,
  XMF_SCORE,
};

/* Use this function to register forwards */
native register_statsfwd(ftype);

/* Function is called after player to player attacks ,
* if players were damaged by teammate TA is set to 1 */
forward client_damage(attacker, victim, damage, wpnindex, hitplace, TA);

/**
 * KTP: Pre-damage forward for damage modification.
 * Called BEFORE damage is finalized - return value modifies the damage.
 *
 * @param attacker   Attacker player index (0 for world/environment)
 * @param victim     Victim player index
 * @param damage     Original damage amount
 * @param wpnindex   Weapon ID (DODW_* constants from dodconst.inc)
 * @param hitplace   Hitgroup (1=head, etc.)
 * @param TA         Team attack (1 if attacker and victim are same team)
 * @return           Modified damage value:
 *                   - Return same damage value to keep unchanged
 *                   - Return lower value to reduce damage (player is healed by difference)
 *                   - Return 0 to completely block damage
 *
 * @note Grenade weapon IDs: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
 *       DODW_HANDGRENADE_EX (37), DODW_STICKGRENADE_EX (38)
 */
forward dod_damage_pre(attacker, victim, damage, wpnindex, hitplace, TA);

/* Function is called after player death ,
* if player was killed by teammate TK is set to 1 */
forward client_death(killer, victim, wpnindex, hitplace, TK);

/* Function is called if player scored  */
forward client_score(id, score, total);

/* This Forward is called when a player changes team */
forward dod_client_changeteam(id, team, oldteam);

/* This Forward is called if a player changes class, but just after spawn */
forward dod_client_changeclass(id, class, oldclass);

/* This Forward is called when a player spawns */
forward dod_client_spawn(id);

/* This will be called whenever a player scopes or unscopes
value = 1 scope up
value = 0 scope down */
forward dod_client_scope(id, value);

/* This will be called whenever a player drops a weapon 
weapon is weapon dropped or picked up 
value = 1 picked up
value = 0 dropped */
forward dod_client_weaponpickup(id, weapon, value);

/* Called whenever the the player goes to or comes from prone position 
value = 1 going down
value = 0 getting up */
forward dod_client_prone(id, value);

/* This will be called whenever a player switches a weapon */
forward dod_client_weaponswitch(id, wpnew, wpnold);

/* Forward for when a grenade explodes and its location */
forward dod_grenade_explosion(id, Float:pos[3], wpnid);

/* Forward for when a rocket explodes and its location */
forward dod_rocket_explosion(id, Float:pos[3], wpnid);

/* Forward for when a player picks up a object */
forward dod_client_objectpickup(id, objid, Float:pos[3], value);

/* Forward for when a users stamina decreases */
forward dod_client_stamina(id, stamina);

/* We want to get just the weapon of whichever type that the player is on him 
Use DODWT_* in dodconst.inc for type */
native dod_weapon_type(id, type);

/* This native will change the position of a weapon within the users slots and its ammo ammount */
native dod_set_weaponlist(id, wpnID, slot, dropslot, totalrds);

/* Sets the model for a player */
native dod_set_model(id, const model[]);

/* Sets the model for a player */
native dod_set_body_number(id, bodynumber);

/* Un-Sets the model for a player */
native dod_clear_model(id);

/* Custom Weapon Support */
/* function will return index of new weapon */
native custom_weapon_add( const wpnname[], melee = 0, const logname[]="" ); 

/* Function will pass damage done by this custom weapon to stats module and other plugins */
native custom_weapon_dmg( weapon, att, vic, damage, hitplace=0 ); 

/* Function will pass info about custom weapon shot to stats module */
native custom_weapon_shot( weapon, index ); // weapon id , player id

/* function will return 1 if true */
native xmod_is_melee_wpn(wpnindex);

/* Returns weapon name. */
native xmod_get_wpnname(wpnindex, name[], len);

/* Returns weapon logname. */
native xmod_get_wpnlogname(wpnindex, name[], len);

/* Returns weapons array size */
native xmod_get_maxweapons();

/* Returns stats array size ex. 8 in TS , 9 in DoD */
native xmod_get_stats_size();

/* Returns 1 if true */
native xmod_is_custom_wpn(wpnindex);

/************* Shared Natives End ********************************/

/* weapon logname to weapon name convertion */
native dod_wpnlog_to_name(const logname[],name[],len);

/* weapon logname to weapon index convertion */
native dod_wpnlog_to_id(const logname[]);

native dod_get_map_info( info );

/* Returns id of currently carried weapon. Gets also 
* ammount of ammo in clip and backpack. */
native dod_get_user_weapon(index,&clip=0,&ammo=0);

/* Returns team score */
native dod_get_team_score(teamId);

/* Returns player class id */
native dod_get_user_class(index);

/* Returns player score */
native dod_get_user_score(index);

/* values are: 0-no prone, 1-prone, 2-prone + w_deploy */
native dod_get_pronestate(index);

/* It is not as safe as original but player deaths will not be increased */
native dod_user_kill(index);

/************* KTP: HLStatsX Integration Natives ********************************/

/**
 * Forward called by dodx_flush_all_stats() for each connected player.
 * Stats logging plugins should register for this to log pending weaponstats.
 *
 * @param id        Player index to flush stats for
 */
forward dod_stats_flush(id);

// =============== KTP: HLStatsX Integration Natives ===============

/**
 * Flush all pending player stats by firing dod_stats_flush forward.
 * Call this before match starts to log warmup stats.
 *
 * @return          Number of players for which stats were flushed
 */
native dodx_flush_all_stats();

/**
 * Reset all accumulated stats for all connected players.
 * Call this after flushing warmup stats to start fresh for match.
 * Clears: weapons[], attackers[], victims[], weaponsLife[], weaponsRnd[], life, round
 *
 * @return          Number of players reset
 */
native dodx_reset_all_stats();

/**
 * Set the match ID for stats logging.
 * When set, weaponstats log lines will include (matchid "xxx") property.
 * Pass empty string to clear match context (for post-match/warmup).
 *
 * @param matchId   Match ID string (e.g., "KTP-1734355200-dod_charlie")
 * @return          1 on success
 */
native dodx_set_match_id(const matchId[]);

/**
 * Get the current match ID.
 *
 * @param output    Buffer to store match ID
 * @param maxlen    Maximum buffer length
 * @return          Length of match ID string, 0 if not set
 */
native dodx_get_match_id(output[], maxlen);

/**
 * Set player's team name in private data.
 * This affects server-side logs but NOT the scoreboard column
 * (DoD client hardcodes "Allies"/"Axis" team names).
 * Works in extension mode (no metamod required).
 *
 * @param id        Player index
 * @param szName    Team name string (max 15 chars)
 * @return          1 on success, 0 on failure
 */
native dodx_set_pl_teamname(id, const szName[]);

// =============== KTP: Gamerules Score Modification ===============

/**
 * Check if gamerules score modification is available.
 * Returns true if dodx_set_team_score/dodx_get_team_score will work.
 * Requires g_pGameRules address to be found via gamedata signature.
 *
 * @return          1 if available, 0 if not
 */
native dodx_has_gamerules();

/**
 * Set team score in gamerules (modifies the scoreboard directly).
 * This allows restoring cumulative scores from 1st half when 2nd half starts.
 * The score is written directly to CDoDTeamPlay::m_iTeamScores array.
 *
 * @param team      Team index (1=Allies, 2=Axis)
 * @param score     Score value to set
 * @return          1 on success, 0 on failure (gamerules not available)
 */
native dodx_set_team_score(team, score);

/**
 * Get team score from gamerules (reads the scoreboard value directly).
 * If gamerules is not available, falls back to DODX message-tracked score.
 *
 * @param team      Team index (1=Allies, 2=Axis)
 * @return          Current team score
 */
native dodx_get_team_score(team);

/**
 * Broadcast TeamScore message to all clients and update gamerules.
 * This properly updates client scoreboards after modifying scores.
 * Use this instead of trying to send TeamScore via AMX message natives.
 *
 * @param team      Team index (1=Allies, 2=Axis)
 * @param score     Score value to broadcast
 * @return          1 on success, 0 on failure
 */
native dodx_broadcast_team_score(team, score);

/**
 * Set custom team name on scoreboard for all players on a team.
 * Sends TeamInfo message to all clients - may override "Allies"/"Axis" display.
 * Call this after players join teams to show custom names on scoreboard.
 *
 * @param team      Team index (1=Allies, 2=Axis)
 * @param name      Custom team name string (e.g., "Team Alpha")
 * @return          Number of players updated
 */
native dodx_set_scoreboard_team_name(team, const name[]);

// =============== KTP: Grenade Ammo Manipulation ===============

/**
 * Set grenade ammo count for a player.
 * Use this to customize grenade loadouts per class on spawn.
 * Works in extension mode (no Metamod/dodfun required).
 *
 * @param id            Player index
 * @param grenade_type  Grenade weapon ID: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
 * @param count         Number of grenades to set (0-10)
 * @return              1 on success, 0 on failure
 *
 * @note DODW_HANDGRENADE and DODW_MILLS_BOMB share the same ammo pool (Allies grenades)
 * @note DODW_STICKGRENADE is the Axis grenade type
 * @note Call this in dod_client_spawn forward with a short delay for best results
 */
native dodx_set_grenade_ammo(id, grenade_type, count);

/**
 * Get current grenade ammo count for a player.
 *
 * @param id            Player index
 * @param grenade_type  Grenade weapon ID: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
 * @return              Current grenade count, or 0 on failure
 */
native dodx_get_grenade_ammo(id, grenade_type);

/**
 * Set player noclip mode.
 * Ported from fun module for extension mode compatibility.
 *
 * @param id       Player index
 * @param noclip   0 = disable noclip (normal walk), 1 = enable noclip
 * @return         1 on success, 0 on failure
 */
native dodx_set_user_noclip(id, noclip);

/**
 * Send AmmoX message to update client HUD ammo display.
 * Use this after modifying grenade ammo to sync the client HUD.
 *
 * @param id         Player index
 * @param ammo_slot  Ammo slot ID: 9 = hand grenade/mills bomb, 11 = stick grenade
 * @param count      Ammo count to display (0-254)
 * @return           1 on success, 0 on failure
 */
native dodx_send_ammox(id, ammo_slot, count);

/**
 * Give a grenade weapon to a player.
 * Creates a weapon entity and makes the player pick it up.
 * Use this for infinite grenade functionality in practice mode.
 *
 * @param id            Player index
 * @param grenade_type  Grenade weapon ID: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
 * @return              1 on success, 0 on failure
 */
native dodx_give_grenade(id, grenade_type);

/**
 * Strip a grenade weapon from a player.
 * Removes the grenade weapon entity and clears all ammo slots.
 * Use this before giving grenades to ensure clean state.
 *
 * @param id            Player index
 * @param grenade_type  Grenade weapon ID: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
 * @return              Number of weapon entities removed, 0 if none found
 */
native dodx_strip_grenade(id, grenade_type);

/**
 * Debug: Dump potential ammo offset values to server log.
 * Use this to find correct ammo offsets on different OS versions.
 *
 * @param id  Player index
 * @return    1 on success
 */
native dodx_debug_dump_ammo(id);

/**
 * Set player class.
 * Ported from dodfun module for extension mode compatibility.
 *
 * @param id        Player index
 * @param classId   Class ID (1-6 depending on team), or 0 for random class
 * @return          1 on success, 0 on failure
 */
native dodx_set_user_class(id, classId);

/**
 * Set player team.
 * Ported from dodfun module for extension mode compatibility.
 * Note: This kills the player and sets random class.
 *
 * @param id        Player index
 * @param teamId    Team ID: 1 = Allies, 2 = Axis, 3 = Spectators
 * @param refresh   1 = broadcast team change to all clients, 0 = silent
 * @return          1 on success, 0 on failure
 */
native dodx_set_user_team(id, teamId, refresh = 1);

/**
 * Get player origin (position).
 * Extension mode compatible alternative to pev/get_user_origin.
 *
 * @param id        Player index
 * @param origin    Float array[3] to store origin (x, y, z)
 * @return          1 on success, 0 on failure
 */
native dodx_get_user_origin(id, Float:origin[3]);

/**
 * Set player origin (position/teleport).
 * Extension mode compatible alternative to set_pev/entity_set_origin.
 *
 * @param id        Player index
 * @param origin    Float array[3] with new origin (x, y, z)
 * @return          1 on success, 0 on failure
 */
native dodx_set_user_origin(id, Float:origin[3]);

/**
 * Get player view angles.
 * Extension mode compatible alternative to pev.
 *
 * @param id        Player index
 * @param angles    Float array[3] to store angles (pitch, yaw, roll)
 * @return          1 on success, 0 on failure
 */
native dodx_get_user_angles(id, Float:angles[3]);

/**
 * Set player view angles.
 * Extension mode compatible alternative to set_pev.
 *
 * @param id        Player index
 * @param angles    Float array[3] with new angles (pitch, yaw, roll)
 * @return          1 on success, 0 on failure
 */
native dodx_set_user_angles(id, Float:angles[3]);

// =============== KTP: Control Point Tracking ===============

/**
 * Control point data keys for dodx_objective_get_data/dodx_objective_set_data.
 */
enum CP_VALUE {
	CP_edict = 1,
	CP_area,
	CP_index,
	CP_owner,
	CP_default_owner,
	CP_visible,
	CP_icon_neutral,
	CP_icon_allies,
	CP_icon_axis,
	CP_origin_x,
	CP_origin_y,

	CP_can_touch,
	CP_pointvalue,

	CP_points_for_cap,
	CP_team_points,

	CP_model_body_neutral,
	CP_model_body_allies,
	CP_model_body_axis,

	// strings
	CP_name,
	CP_cap_message,
	CP_reset_capsound,
	CP_allies_capsound,
	CP_axis_capsound,
	CP_targetname,

	CP_model_neutral,
	CP_model_allies,
	CP_model_axis,
};

/**
 * Capture area data keys for dodx_area_get_data/dodx_area_set_data.
 */
enum CA_VALUE {
	CA_edict = 1,
	CA_allies_numcap,
	CA_axis_numcap,
	CA_timetocap,
	CA_can_cap,

	// strings
	CA_target,
	CA_sprite,
};

/**
 * Called after map control point data is initialized.
 * Use dodx_objectives_get_num() and dodx_objective_get_data() to query CP state.
 */
forward controlpoints_init();

/**
 * Called when a control point changes ownership.
 *
 * @param cp_index    Control point index (0-based)
 * @param new_owner   New owning team (0=neutral, 1=allies, 2=axis)
 * @param old_owner   Previous owning team
 */
forward dod_control_point_captured(cp_index, new_owner, old_owner);

/**
 * Called when a player receives score points, enriched with CP context.
 * Fires alongside client_score but includes which control point triggered the score.
 *
 * @param id          Player index
 * @param score_delta Points earned (positive delta)
 * @param total_score Player's new total score
 * @param cp_index    Control point index that triggered the score, or -1 if not CP-related
 */
forward dod_score_event(id, score_delta, total_score, cp_index);

/**
 * Get the number of control points on the current map.
 *
 * @return          Number of control points (0 if not yet initialized)
 */
native dodx_objectives_get_num();

/**
 * Get control point data.
 *
 * @param index     CP array index (0 to dodx_objectives_get_num()-1)
 * @param key       CP_VALUE key to query
 * @param szvalue   Buffer for string values (CP_name, CP_cap_message, etc.)
 * @param len       Buffer length for string values
 * @return          Integer value for int keys, 1 for string keys
 */
native dodx_objective_get_data(index, CP_VALUE:key, szvalue[] = "", len = 0);

/**
 * Set control point data.
 *
 * @param index     CP array index (0 to dodx_objectives_get_num()-1)
 * @param key       CP_VALUE key to set
 * @param ivalue    Integer value for int keys
 * @param szvalue   String value for string keys
 * @return          1 on success, 0 on failure
 */
native dodx_objective_set_data(index, CP_VALUE:key, ivalue = 0, const szvalue[] = "");

/**
 * Re-send InitObj message to refresh CP HUD display.
 *
 * @param player    Player index (0 = all players)
 * @return          1 on success, 0 on failure
 */
native dodx_objectives_reinit(player = 0);

/**
 * Get capture area data for a control point.
 *
 * @param index     CP array index (0 to dodx_objectives_get_num()-1)
 * @param key       CA_VALUE key to query
 * @param szvalue   Buffer for string values
 * @param len       Buffer length for string values
 * @return          Integer value for int keys, 1 for string keys, 0 if no area
 */
native dodx_area_get_data(index, CA_VALUE:key, szvalue[] = "", len = 0);

/**
 * Set capture area data for a control point.
 *
 * @param index     CP array index (0 to dodx_objectives_get_num()-1)
 * @param key       CA_VALUE key to set
 * @param ivalue    Integer value for int keys
 * @param szvalue   String value for string keys
 * @return          1 on success, 0 if no area
 */
native dodx_area_set_data(index, CA_VALUE:key, ivalue = 0, const szvalue[] = "");

// ==================================================================
