// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
// Copyright (C) 2004 Lukasz Wlasinski.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

//
// DODX Module
//

#include "amxxmodule.h"
#include "dodx.h"

/* Weapon names aren't send in WeaponList message in DoD */
weaponlist_s weaponlist[] =
{
	{ 0,     0,	  0,	false}, // 0,
	{ -1,    0,	 -1,	true }, // DODW_AMERKNIFE = 1,
	{ -1,    0,	 -1,	true }, // DODW_GERKNIFE,
	{  4,   64,	  7,	true }, // DODW_COLT,
	{  4,   64,	  8,	true }, // DODW_LUGER,
	{  3,  128,	  8,	true }, // DODW_GARAND,
	{  3,  128,	  5,	true }, // DODW_SCOPED_KAR,
	{  1,  128,	 30,	true }, // DODW_THOMPSON,
	{  6,  128,	 30,	true }, // DODW_STG44,
	{  5,  128,	  5,	true }, // DODW_SPRINGFIELD,
	{  3,  128,	  5,	true }, // DODW_KAR,
	{  6,  128,	 20,	true }, // DODW_BAR,
	{  1,  130,	 30,	true }, // DODW_MP40,
	{  9,   24,	 -1,	true }, // DODW_HANDGRENADE,
	{ 11,   24,	 -1,	true }, // DODW_STICKGRENADE,
	{ 12,   24,	 -1,	true }, // DODW_STICKGRENADE_EX,
	{ 10,   24,	 -1,	true }, // DODW_HANDGRENADE_EX,
	{  7, 2178,	250,	true }, // DODW_MG42,
	{  8,  130,	150,	true }, // DODW_30_CAL,
	{ -1,    0,	 -1,	true }, // DODW_SPADE,
	{  2,  128,	 15,	true }, // DODW_M1_CARBINE,
	{  2,  130,	 75,	true }, // DODW_MG34,
	{  1,  128,	 30,	true }, // DODW_GREASEGUN,
	{  6,  128,	 20,	true }, // DODW_FG42,
	{  2,  128,	 10,	true }, // DODW_K43,
	{  3,  128,	 10,	true }, // DODW_ENFIELD,
	{  1,  128,	 30,	true }, // DODW_STEN,
	{  6,  128,	 30,	true }, // DODW_BREN,
	{  4,   64,	  6,	true }, // DODW_WEBLEY,
	{ 13,  642,	  1,	true }, // DODW_BAZOOKA,
	{ 13,  642,	  1,	true }, // DODW_PANZERSCHRECK,
	{ 13,  642,	  1,	true }, // DODW_PIAT,
	{  3,  128,	 20,	true }, // DODW_SCOPED_FG42, UNSURE ABOUT THIS ONE
	{  2,  128,	 15,	true }, // DODW_FOLDING_CARBINE,
	{  0,    0,	  0,	false}, // DODW_KAR_BAYONET,
	{  3,  128,	 10,	true }, // DODW_SCOPED_ENFIELD, UNSURE ABOUT THIS ONE
	{  9,   24,	 -1,	true }, // DODW_MILLS_BOMB,
	{ -1,    0,	 -1,	true }, // DODW_BRITKNIFE,
	{ 38,    0,	  0,	false}, // DODW_GARAND_BUTT,
	{ 39,    0,	  0,	false}, // DODW_ENFIELD_BAYONET,
	{ 40,    0,	  0,	false}, // DODW_MORTAR,
	{ 41,    0,	  0,	false}, // DODW_K43_BUTT,
};

#define WEAPONLIST_SIZE (sizeof(weaponlist) / sizeof(weaponlist[0]))

// from id to name 3 params id, name, len
static cell AMX_NATIVE_CALL get_weapon_name(AMX *amx, cell *params)
{ 
	int id = params[1];

	if(id < 0 || id >= DODMAX_WEAPONS)
	{ 
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id %d", id);
		return 0;
	}

	return MF_SetAmxString(amx,params[2],weaponData[id].name,params[3]);
}

// from log to name
static cell AMX_NATIVE_CALL wpnlog_to_name(AMX *amx, cell *params)
{ 
	int iLen;
	char *log = MF_GetAmxString(amx,params[1],0,&iLen);

	for(int i = 0; i < DODMAX_WEAPONS; i++)
	{
		if(strcmp(log,weaponData[i].logname ) == 0)
			return MF_SetAmxString(amx,params[2],weaponData[i].name,params[3]);
	}
	return 0;
}

// from log to id
static cell AMX_NATIVE_CALL wpnlog_to_id(AMX *amx, cell *params)
{ 
	int iLen;
	char *log = MF_GetAmxString(amx, params[1], 0, &iLen);

	for(int i = 0; i < DODMAX_WEAPONS; i++)
	{
		if(strcmp(log,weaponData[i].logname) == 0)
			return i;
	}
	return 0;
}

// from id to log
static cell AMX_NATIVE_CALL get_weapon_logname(AMX *amx, cell *params)
{ 
	int id = params[1];

	if (id<0 || id>=DODMAX_WEAPONS)
	{ 
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id %d", id);
		return 0;
	}

	return MF_SetAmxString(amx,params[2],weaponData[id].logname,params[3]);
}

static cell AMX_NATIVE_CALL is_melee(AMX *amx, cell *params)
{
	int id = params[1];

	if(id < 0 || id >= DODMAX_WEAPONS)
	{ 
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id %d", id);
		return 0;
	}

	return weaponData[id].melee;
}

static cell AMX_NATIVE_CALL get_team_score(AMX *amx, cell *params)
{
	int index = params[1];

	switch ( index )
	{
	case 1:
		return AlliesScore;
		break;

	case 2:
		return AxisScore;
		break;
	}
	return 0;
}

static cell AMX_NATIVE_CALL get_user_score(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	if (pPlayer->ingame)
		return (cell)pPlayer->savedScore;

	return -1;
}

static cell AMX_NATIVE_CALL get_user_class(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	// KTP: Check pEdict is valid before accessing
	if (pPlayer->ingame && pPlayer->pEdict && !pPlayer->pEdict->free)
		return pPlayer->pEdict->v.playerclass;

	return 0;
}

// KTP: Set player class (ported from dodfun, extension mode compatible)
static cell AMX_NATIVE_CALL dodx_set_user_class(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	int iClass = params[2];

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->ingame || !pPlayer->pEdict || pPlayer->pEdict->free || !pPlayer->pEdict->pvPrivateData)
		return 0;

	if (iClass) {
		*((int*)pPlayer->pEdict->pvPrivateData + STEAM_PDOFFSET_CLASS) = iClass;
		*((int*)pPlayer->pEdict->pvPrivateData + STEAM_PDOFFSET_RCLASS) = 0; // disable random class
	} else {
		*((int*)pPlayer->pEdict->pvPrivateData + STEAM_PDOFFSET_RCLASS) = 1; // set random class
	}

	return 1;
}

// KTP: Set player team (ported from dodfun, extension mode compatible)
static cell AMX_NATIVE_CALL dodx_set_user_team(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	int iTeam = params[2];

	if (iTeam < 1 || iTeam > 3) {
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid team id %d", iTeam);
		return 0;
	}

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->ingame || !pPlayer->pEdict || pPlayer->pEdict->free || !pPlayer->pEdict->pvPrivateData)
		return 0;

	pPlayer->killPlayer();
	pPlayer->pEdict->v.team = iTeam;

	// Set team name in private data
	char* pTeamName = (char*)pPlayer->pEdict->pvPrivateData + STEAM_PDOFFSET_TEAMNAME;
	const char* teamName;
	switch (iTeam) {
		case 1: teamName = "Allies"; break;
		case 2: teamName = "Axis"; break;
		case 3: teamName = "Spectators"; break;
		default: teamName = ""; break;
	}
	strncpy(pTeamName, teamName, 15);
	pTeamName[15] = '\0';

	*((int*)pPlayer->pEdict->pvPrivateData + STEAM_PDOFFSET_RCLASS) = 1; // set random class

	// Broadcast team change if refresh requested
	if (params[3]) {
		MESSAGE_BEGIN(MSG_ALL, gmsgPTeam);
		WRITE_BYTE(pPlayer->index);
		WRITE_BYTE(iTeam);
		MESSAGE_END();
	}

	return 1;
}

// KTP: Get player origin (extension mode compatible, no fakemeta needed)
static cell AMX_NATIVE_CALL dodx_get_user_origin(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->ingame || !pPlayer->pEdict || pPlayer->pEdict->free)
		return 0;

	cell *origin = MF_GetAmxAddr(amx, params[2]);
	origin[0] = amx_ftoc(pPlayer->pEdict->v.origin[0]);
	origin[1] = amx_ftoc(pPlayer->pEdict->v.origin[1]);
	origin[2] = amx_ftoc(pPlayer->pEdict->v.origin[2]);

	return 1;
}

// KTP: Set player origin (extension mode compatible, no fakemeta needed)
static cell AMX_NATIVE_CALL dodx_set_user_origin(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->ingame || !pPlayer->pEdict || pPlayer->pEdict->free)
		return 0;

	cell *origin = MF_GetAmxAddr(amx, params[2]);
	pPlayer->pEdict->v.origin[0] = amx_ctof(origin[0]);
	pPlayer->pEdict->v.origin[1] = amx_ctof(origin[1]);
	pPlayer->pEdict->v.origin[2] = amx_ctof(origin[2]);

	return 1;
}

// KTP: Get player view angles (extension mode compatible, no fakemeta needed)
static cell AMX_NATIVE_CALL dodx_get_user_angles(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->ingame || !pPlayer->pEdict || pPlayer->pEdict->free)
		return 0;

	cell *angles = MF_GetAmxAddr(amx, params[2]);
	angles[0] = amx_ftoc(pPlayer->pEdict->v.v_angle[0]);
	angles[1] = amx_ftoc(pPlayer->pEdict->v.v_angle[1]);
	angles[2] = amx_ftoc(pPlayer->pEdict->v.v_angle[2]);

	return 1;
}

// KTP: Set player view angles (extension mode compatible, no fakemeta needed)
static cell AMX_NATIVE_CALL dodx_set_user_angles(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->ingame || !pPlayer->pEdict || pPlayer->pEdict->free)
		return 0;

	cell *angles = MF_GetAmxAddr(amx, params[2]);
	pPlayer->pEdict->v.v_angle[0] = amx_ctof(angles[0]);
	pPlayer->pEdict->v.v_angle[1] = amx_ctof(angles[1]);
	pPlayer->pEdict->v.v_angle[2] = amx_ctof(angles[2]);

	// Also set angles for proper view direction
	pPlayer->pEdict->v.angles[0] = amx_ctof(angles[0]) / -3.0f;
	pPlayer->pEdict->v.angles[1] = amx_ctof(angles[1]);
	pPlayer->pEdict->v.angles[2] = 0;

	// Fix view with fixangle
	pPlayer->pEdict->v.fixangle = 1;

	return 1;
}

static cell AMX_NATIVE_CALL user_kill(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	if(pPlayer->ingame && pPlayer->IsAlive())
	{
		pPlayer->killPlayer();
		return 1;
	}

	return 0;
}	

static cell AMX_NATIVE_CALL get_map_info(AMX *amx, cell *params)
{
	switch(params[1])
	{
	case 0:
		return g_map.detect_allies_country;
		break;

	case 1:
		return g_map.detect_allies_paras;
		break;

	case 2:
		return g_map.detect_axis_paras;
		break;

	default:
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid map info id %d", params[1]);
		break;
	}
	return -1;
}

static cell AMX_NATIVE_CALL get_user_pronestate(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	// KTP: Check pEdict is valid before accessing
	if (pPlayer->ingame && pPlayer->pEdict && !pPlayer->pEdict->free)
		return pPlayer->pEdict->v.iuser3;

	return 0;
}

static cell AMX_NATIVE_CALL get_user_weapon(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	if (pPlayer->ingame)
	{
		int wpn = pPlayer->current;
		cell *cpTemp = MF_GetAmxAddr(amx,params[2]);
		*cpTemp = pPlayer->weapons[wpn].clip;
		cpTemp = MF_GetAmxAddr(amx,params[3]);
		*cpTemp = pPlayer->weapons[wpn].ammo;
		return wpn;
	}

	return 0;
}

/* We want to get just the weapon of whichever type that the player is on him */
static cell AMX_NATIVE_CALL dod_weapon_type(AMX *amx, cell *params) /* 2 params */
{
	int index = params[1];
	int type = params[2];

	CHECK_PLAYER(index);

	if(type < DODWT_PRIMARY || type > DODWT_OTHER)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon type id %d", type);
		return 0;
	}

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	// KTP: Check pEdict is valid before accessing
	if(pPlayer->ingame && pPlayer->pEdict && !pPlayer->pEdict->free)
	{
		int weaponsbit = pPlayer->pEdict->v.weapons & ~(1<<31); // don't count last element

		for(int x = 1; x < DODMAX_WEAPONS; ++x)
		{
			if((weaponsbit&(1<<x)) > 0)
			{
				if(weaponData[x].type == type)
					return x;
			}
		}
	}

	return 0;
}

// forward
static cell AMX_NATIVE_CALL register_forward(AMX *amx, cell *params)
{ 

	#ifdef FORWARD_OLD_SYSTEM
		int iFunctionIndex;
		int err;
		switch( params[1] )
		{
		case 0:
			if((err = MF_AmxFindPublic(amx, "client_damage", &iFunctionIndex)) == AMX_ERR_NONE)
				g_damage_info.put( amx , iFunctionIndex );

			else
				MF_LogError(amx, err, "client_damage not found");
				return 0;
			break;

		case 1:
			if((err = MF_AmxFindPublic(amx, "client_death", &iFunctionIndex)) == AMX_ERR_NONE)
				g_death_info.put( amx , iFunctionIndex );

			else
				MF_LogError(amx, err, "client_Death not found");
				return 0;
			break;

		case 2:
			if((err = MF_AmxFindPublic(amx, "client_score", &iFunctionIndex)) == AMX_ERR_NONE)
				g_score_info.put( amx , iFunctionIndex );

			else
				MF_LogError(amx, err, "client_score not found");
				return 0;
			break;

		default:
			MF_LogError(amx, AMX_ERR_NATIVE, "Invalid forward id %d", params[2]);
			return 0;
		}
	#endif

	return 1;
}

// name,logname,melee=0 
static cell AMX_NATIVE_CALL register_cwpn(AMX *amx, cell *params)
{ 
	int i;
	bool bFree = false;

	for(i = DODMAX_WEAPONS - DODMAX_CUSTOMWPNS; i < DODMAX_WEAPONS; i++)
	{
		if(!weaponData[i].needcheck)
		{
			bFree = true;
			break;
		}
	}

	if(!bFree)
		return 0;

	int iLen;
	char *szName = MF_GetAmxString(amx, params[1], 0, &iLen);
	char *szLogName = MF_GetAmxString(amx, params[3], 0, &iLen);

	strncpy(weaponData[i].name, szName, sizeof(weaponData[i].name) - 1);
	weaponData[i].name[sizeof(weaponData[i].name) - 1] = '\0';
	strncpy(weaponData[i].logname, szLogName, sizeof(weaponData[i].logname) - 1);
	weaponData[i].logname[sizeof(weaponData[i].logname) - 1] = '\0';
	weaponData[i].needcheck = true;
	weaponData[i].melee = params[2] ? true:false;
	return i;
}

// wid,att,vic,dmg,hp=0
static cell AMX_NATIVE_CALL cwpn_dmg(AMX *amx, cell *params)
{ 
	int weapon = params[1];

	// only for custom weapons
	if(weapon < DODMAX_WEAPONS-DODMAX_CUSTOMWPNS)
	{ 
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid custom weapon id %d", weapon);
		return 0;
	}

	int att = params[2];
	CHECK_PLAYER(params[2]);

	int vic = params[3];
	CHECK_PLAYER(params[3]);
	
	int dmg = params[4];
	if(dmg<1)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid damage %d", dmg);
		return 0;
	}
	
	int aim = params[5];
	if(aim < 0 || aim > 7)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid aim %d", aim);
		return 0;
	}

	CPlayer* pAtt = GET_PLAYER_POINTER_I(att);
	CPlayer* pVic = GET_PLAYER_POINTER_I(vic);

	// KTP: Check pEdict is valid before accessing
	if (!pVic->pEdict || pVic->pEdict->free)
		return 0;
	if (!pAtt->pEdict || pAtt->pEdict->free)
		return 0;

	pVic->pEdict->v.dmg_inflictor = NULL;

	if(pAtt->index != pVic->index)
		pAtt->saveHit(pVic , weapon , dmg, aim);

	int TA = 0;

	if((pVic->pEdict->v.team == pAtt->pEdict->v.team) && (pVic != pAtt))
		TA = 1;

	MF_ExecuteForward(iFDamage,pAtt->index, pVic->index, dmg, weapon, aim, TA);

	if(pVic->IsAlive())
		return 1;

	pAtt->saveKill(pVic,weapon,( aim == 1 ) ? 1:0 ,TA);

	MF_ExecuteForward(iFDeath,pAtt->index, pVic->index, weapon, aim, TA);

	return 1;
}

// player,wid
static cell AMX_NATIVE_CALL cwpn_shot(AMX *amx, cell *params)
{ 
	int index = params[2];

	CHECK_PLAYER(index);

	int weapon = params[1];
	if(weapon < DODMAX_WEAPONS-DODMAX_CUSTOMWPNS)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid custom weapon id %d", weapon);
		return 0;
	}

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	pPlayer->saveShot(weapon);

	return 1;
}

static cell AMX_NATIVE_CALL get_maxweapons(AMX *amx, cell *params)
{
	return DODMAX_WEAPONS;
}

static cell AMX_NATIVE_CALL get_stats_size(AMX *amx, cell *params)
{
	return 9;
}

static cell AMX_NATIVE_CALL is_custom(AMX *amx, cell *params)
{
	int weapon = params[1];

	if(weapon < DODMAX_WEAPONS-DODMAX_CUSTOMWPNS)
	{
		return 0;
	}
	return 1;
}

// player,wid
static cell AMX_NATIVE_CALL dod_get_user_team(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	// KTP: Check pEdict is valid before accessing
	if (!pPlayer->pEdict || pPlayer->pEdict->free)
		return 0;
	return pPlayer->pEdict->v.team;

}

// player,wid
// KTP: This function is disabled in extension mode - core AMXX's get_user_team is used instead
// The native registration for this function is commented out in the natives table below
static cell AMX_NATIVE_CALL get_user_team(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	// KTP: Check pEdict is valid before accessing
	if (!pPlayer->pEdict || pPlayer->pEdict->free)
		return 0;
	int iTeam = pPlayer->pEdict->v.team;

	if ( params[3] )
	{
		const char *szTeam = "";
		switch(iTeam)
		{
		case 1:
			szTeam = "Allies";
			break;

		case 2:
			szTeam = "Axis";
			break;
		}

		MF_SetAmxString(amx,params[2],szTeam,params[3]);
	}
	return iTeam;
}

static cell AMX_NATIVE_CALL dod_set_model(AMX *amx, cell *params) // player,model
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if(!pPlayer->ingame)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid Player, Not on Server");
		return 0;
	}

	int length;
	pPlayer->initModel((char*)STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 1, &length))));

	return true;
}

static cell AMX_NATIVE_CALL dod_set_body(AMX *amx, cell *params) // player,bodynumber
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if(!pPlayer->ingame)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid Player, Not on Server");
		return 0;
	}

	pPlayer->setBody(params[2]);

	return true;
}

static cell AMX_NATIVE_CALL dod_clear_model(AMX *amx, cell *params) // player
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if(!pPlayer->ingame)
		return false;

	pPlayer->clearModel();

	return true;
}

/* 
0 [Byte]	1	// Weapons Groupings
1 [Byte]	210	// Total Rounds Allowed
2 [Byte]	-1	// Undefined Not Used
3 [Byte]	-1	// Undefined Not Used
4 [Byte]	2	// Weapon Slot
5 [Byte]	0	// Bucket ( Position Under Weapon Slot )
6 [Short]	7	// Weapon Number / Bit Field for the weapon
7 [Byte]	128	// Bit Field for the Ammo or Ammo Type
8 [Byte]	30	// Rounds Per Mag

id, wpnID, slot, position, totalrds
*/
static cell AMX_NATIVE_CALL dod_weaponlist(AMX *amx, cell *params) // player
{
	int id = params[1];
	int wpnID = params[2];
	int slot = params[3];
	int position = params[4];
	int totalrds = params[5];

	// Bounds check both indices before array access
	if (id < 0 || id >= (int)WEAPONLIST_SIZE)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id %d (max %d)", id, (int)WEAPONLIST_SIZE - 1);
		return 0;
	}
	if (wpnID < 0 || wpnID >= (int)WEAPONLIST_SIZE)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid wpnID %d (max %d)", wpnID, (int)WEAPONLIST_SIZE - 1);
		return 0;
	}

	if(!weaponlist[id].changeable)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "This Weapon Cannot be Changed");
		return 0;
	}

	UTIL_LogPrintf("ID (%d) WpnID (%d) Slot (%d) Pos (%d) Rounds (%d)", id, wpnID, slot, position, totalrds);

	CHECK_PLAYER(id);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(id);
	if(!pPlayer->ingame)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid Player, Not on Server");
		return 0;
	}

	MESSAGE_BEGIN(MSG_ONE, GET_USER_MSG_ID(PLID, "WeaponList", NULL), NULL, pPlayer->pEdict);
	WRITE_BYTE(weaponlist[wpnID].grp);
		WRITE_BYTE(totalrds);
		WRITE_BYTE(-1);
		WRITE_BYTE(-1);
		WRITE_BYTE(slot - 1);
		WRITE_BYTE(position);
		WRITE_SHORT(wpnID);
		WRITE_BYTE(weaponlist[wpnID].bitfield);

		// Is it grenades
		if(wpnID == 13 || wpnID == 14 || wpnID == 15 || wpnID == 16 || wpnID == 36)
			WRITE_BYTE(-1);
		else if(wpnID == 29 || wpnID == 30 || wpnID == 31)
			WRITE_BYTE(1);
		else
			WRITE_BYTE(weaponlist[wpnID].clip);
	MESSAGE_END();

	return 1;
}

// KTP: Set player's team name in private data (extension mode compatible)
// This affects server-side logs but NOT the scoreboard (DoD client hardcodes team names)
// native dodx_set_pl_teamname(id, const szName[]);
static cell AMX_NATIVE_CALL dodx_set_pl_teamname(AMX *amx, cell *params)
{
	int id = params[1];
	if (id < 1 || id > gpGlobals->maxClients)
		return 0;

	edict_t* pEdict = MF_GetPlayerEdict(id);
	if (!pEdict || !pEdict->pvPrivateData)
		return 0;

	int len;
	const char* szName = MF_GetAmxString(amx, params[2], 0, &len);

	// Copy exactly 16 bytes like dodfun does (null-padded)
	char nameBuf[16] = {0};
	int copyLen = (len < 15) ? len : 15;
	memcpy(nameBuf, szName, copyLen);

	// Copy all 16 bytes to private data
	char* pTeamName = (char*)pEdict->pvPrivateData + STEAM_PDOFFSET_TEAMNAME;
	for (int i = 0; i < 16; i++) {
		pTeamName[i] = nameBuf[i];
	}

	return 1;
}

// KTP: Set team score in gamerules (modifies the scoreboard directly)
// This allows restoring cumulative scores from 1st half when 2nd half starts
// native dodx_set_team_score(team, score);
static cell AMX_NATIVE_CALL dodx_set_team_score(AMX *amx, cell *params)
{
	// Check if gamerules is available
	if (!DODX_HasGameRules())
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_set_team_score: gamerules not available");
		return 0;
	}

	int team = params[1];   // 1=Allies, 2=Axis
	int score = params[2];

	if (team < 1 || team > 2)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_set_team_score: invalid team %d (must be 1 or 2)", team);
		return 0;
	}

	// m_iTeamScores is int[32] at offset g_iTeamScoreOffset in gamerules
	// Team indices in DoD: 1=Allies, 2=Axis (same as array index)
	int *pScores = (int*)((char*)*g_pGameRulesAddress + g_iTeamScoreOffset);
	pScores[team] = score;

	return 1;
}

// KTP: Get team score from gamerules (reads the scoreboard value directly)
// native dodx_get_team_score(team);
static cell AMX_NATIVE_CALL dodx_get_team_score(AMX *amx, cell *params)
{
	// Check if gamerules is available
	if (!DODX_HasGameRules())
	{
		// Fallback to DODX tracked score (from TeamScore message)
		int team = params[1];
		switch (team)
		{
		case 1: return AlliesScore;
		case 2: return AxisScore;
		default: return 0;
		}
	}

	int team = params[1];   // 1=Allies, 2=Axis

	if (team < 1 || team > 2)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_get_team_score: invalid team %d (must be 1 or 2)", team);
		return 0;
	}

	// m_iTeamScores is int[32] at offset g_iTeamScoreOffset in gamerules
	int *pScores = (int*)((char*)*g_pGameRulesAddress + g_iTeamScoreOffset);
	return pScores[team];
}

// KTP: Check if gamerules score modification is available
// native dodx_has_gamerules();
static cell AMX_NATIVE_CALL dodx_has_gamerules(AMX *amx, cell *params)
{
	return DODX_HasGameRules() ? 1 : 0;
}

// KTP: Broadcast TeamScore message to all clients
// This properly updates client scoreboards after modifying gamerules scores
// native dodx_broadcast_team_score(team, score);
static cell AMX_NATIVE_CALL dodx_broadcast_team_score(AMX *amx, cell *params)
{
	int team = params[1];   // 1=Allies, 2=Axis
	int score = params[2];

	if (team < 1 || team > 2)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_broadcast_team_score: invalid team %d (must be 1 or 2)", team);
		return 0;
	}

	// First, set the gamerules score if available
	if (DODX_HasGameRules())
	{
		int *pScores = (int*)((char*)*g_pGameRulesAddress + g_iTeamScoreOffset);
		pScores[team] = score;
	}

	// Update DODX tracked score
	if (team == 1)
		AlliesScore = score;
	else
		AxisScore = score;

	// Check if we have the TeamScore message ID
	if (gmsgTeamScore <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_broadcast_team_score: TeamScore message not registered");
		return 0;
	}

	// Send TeamScore message to all clients
	// DoD TeamScore format: BYTE(team) + SHORT(score)
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore, NULL);
	WRITE_BYTE(team);
	WRITE_SHORT(score);
	MESSAGE_END();

	return 1;
}

// KTP: Set custom team name on scoreboard for all players on a team
// Sends TeamInfo message to all clients for each player on the team
// native dodx_set_scoreboard_team_name(team, const name[]);
static cell AMX_NATIVE_CALL dodx_set_scoreboard_team_name(AMX *amx, cell *params)
{
	int team = params[1];   // 1=Allies, 2=Axis

	if (team < 1 || team > 2)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_set_scoreboard_team_name: invalid team %d (must be 1 or 2)", team);
		return 0;
	}

	// Check if we have the TeamInfo message ID
	if (gmsgTeamInfo <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_set_scoreboard_team_name: TeamInfo message not registered");
		return 0;
	}

	// Get team name string from params
	int len;
	char *teamName = MF_GetAmxString(amx, params[2], 0, &len);
	if (!teamName || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_set_scoreboard_team_name: empty team name");
		return 0;
	}

	int count = 0;

	// Iterate through all players
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CPlayer *pPlayer = GET_PLAYER_POINTER_I(i);
		if (!pPlayer || !pPlayer->pEdict || pPlayer->pEdict->free)
			continue;

		// Get player's team from edict
		int playerTeam = pPlayer->pEdict->v.team;
		if (playerTeam != team)
			continue;

		// Send TeamInfo message to ALL clients for this player
		// TeamInfo format: BYTE(player index) + STRING(team name)
		MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo, NULL);
		WRITE_BYTE(i);
		WRITE_STRING(teamName);
		MESSAGE_END();

		count++;
	}

	return count;
}

// KTP: Grenade ammo manipulation (ported from dodfun for extension mode)
// dodx_set_grenade_ammo(id, grenade_type, count)
// grenade_type: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
static cell AMX_NATIVE_CALL dodx_set_grenade_ammo(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	if (!pPlayer->ingame || !pPlayer->pEdict || !pPlayer->pEdict->pvPrivateData)
		return 0;

	int grenadeType = params[2];
	int count = params[3];

	// Clamp count to reasonable range
	if (count < 0) count = 0;
	if (count > 10) count = 10;

#if defined(__linux__) || defined(__APPLE__)
	if (!g_bPdataOffsetDetected)
	{
		// Phase 1: Offset not yet determined. Write to BOTH +4 and +5 offsets
		// to ensure the correct one gets the value. Then try to detect.
		DODX_PdataWriteBoth(pPlayer->pEdict, grenadeType, count);
		// Try to detect now (may defer if data is insufficient)
		DODX_DetectPdataOffset(pPlayer->pEdict);
		return 1;
	}
#endif

	switch (grenadeType)
	{
		case 13: // DODW_HANDGRENADE
		case 36: // DODW_MILLS_BOMB (shares ammo pool with hand grenade)
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_1) = count;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_2) = count;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_3) = count;
			break;

		case 14: // DODW_STICKGRENADE
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_1) = count;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_2) = count;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_3) = count;
			break;

		default:
			MF_LogError(amx, AMX_ERR_NATIVE, "dodx_set_grenade_ammo: invalid grenade type %d", grenadeType);
			return 0;
	}

	return 1;
}

// dodx_get_grenade_ammo(id, grenade_type)
// Returns current grenade count for the specified type
static cell AMX_NATIVE_CALL dodx_get_grenade_ammo(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);
	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);

	if (!pPlayer->ingame || !pPlayer->pEdict || !pPlayer->pEdict->pvPrivateData)
		return 0;

#if defined(__linux__) || defined(__APPLE__)
	// Auto-detect pdata offset on first grenade operation
	if (!g_bPdataOffsetDetected)
		DODX_DetectPdataOffset(pPlayer->pEdict);
#endif

	int grenadeType = params[2];

	switch (grenadeType)
	{
		case 13: // DODW_HANDGRENADE
		case 36: // DODW_MILLS_BOMB
			return *((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_1);

		case 14: // DODW_STICKGRENADE
			return *((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_1);

		default:
			MF_LogError(amx, AMX_ERR_NATIVE, "dodx_get_grenade_ammo: invalid grenade type %d", grenadeType);
			return 0;
	}
}

// KTP: Noclip control (ported from fun module for extension mode)
// dodx_set_user_noclip(id, noclip)
// noclip: 0 = disable, 1 = enable
static cell AMX_NATIVE_CALL dodx_set_user_noclip(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->pEdict || !pPlayer->pEdict->pvPrivateData)
		return 0;

	// MOVETYPE_WALK = 3, MOVETYPE_NOCLIP = 8
	pPlayer->pEdict->v.movetype = params[2] ? 8 : 3;

	return 1;
}

// KTP: Send AmmoX message to update client HUD
// dodx_send_ammox(id, ammo_slot, count)
// ammo_slot: 9 = hand grenade/mills bomb, 11 = stick grenade
static cell AMX_NATIVE_CALL dodx_send_ammox(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->pEdict)
		return 0;

	if (gmsgAmmoX <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "dodx_send_ammox: AmmoX message not registered");
		return 0;
	}

	int ammoSlot = params[2];
	int count = params[3];

	// Clamp count to byte range
	if (count < 0) count = 0;
	if (count > 254) count = 254;

	MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, NULL, pPlayer->pEdict);
	WRITE_BYTE(ammoSlot);
	WRITE_BYTE(count);
	MESSAGE_END();

	return 1;
}

// KTP: Give a grenade weapon to a player (for infinite grenades in practice mode)
// dodx_give_grenade(id, grenade_type)
// grenade_type: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
static cell AMX_NATIVE_CALL dodx_give_grenade(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->pEdict || !pPlayer->IsAlive())
		return 0;

	int grenadeType = params[2];

	// Determine weapon classname based on grenade type
	const char* weaponClass;
	switch (grenadeType)
	{
		case 13: // DODW_HANDGRENADE
			weaponClass = "weapon_handgrenade";
			break;
		case 14: // DODW_STICKGRENADE
			weaponClass = "weapon_stickgrenade";
			break;
		case 36: // DODW_MILLS_BOMB
			weaponClass = "weapon_mills_bomb";
			break;
		default:
			MF_LogError(amx, AMX_ERR_NATIVE, "dodx_give_grenade: invalid grenade type %d", grenadeType);
			return 0;
	}

	// Get game DLL functions (works in both Metamod and extension mode)
	DLL_FUNCTIONS* pGameDll = (DLL_FUNCTIONS*)MF_GetGameDllFuncs();
	if (!pGameDll || !pGameDll->pfnSpawn || !pGameDll->pfnTouch)
		return 0;

	// Create the weapon entity
	edict_t* pWeapon = CREATE_NAMED_ENTITY(ALLOC_STRING(weaponClass));
	if (!pWeapon || FNullEnt(pWeapon))
		return 0;

	// Position at player's origin
	pWeapon->v.origin = pPlayer->pEdict->v.origin;
	pWeapon->v.spawnflags |= (1 << 30);  // SF_NORESPAWN - prevent respawn

	// Spawn the entity using game DLL function
	pGameDll->pfnSpawn(pWeapon);

	// Remember solid state AFTER spawn but BEFORE touch — pfnSpawn changes
	// solid (e.g. SOLID_NOT -> SOLID_TRIGGER), so capturing before spawn
	// would make the post-touch comparison always differ, leaking entities
	int oldSolid = pWeapon->v.solid;

	// Touch the player to pick it up using game DLL function
	pGameDll->pfnTouch(pWeapon, pPlayer->pEdict);

	// If solid state changed, pickup was successful
	// If not, entity wasn't picked up - remove it to avoid clutter
	if (pWeapon->v.solid == oldSolid && !FNullEnt(pWeapon) && pWeapon->free == 0)
	{
		REMOVE_ENTITY(pWeapon);
		return -1;  // Indicate pickup failed (player may already have max)
	}

	return 1;
}

// dodx_strip_grenade(id, grenade_type)
// Clears grenade ammo for a player (simplified - just zeros ammo slots)
// grenade_type: DODW_HANDGRENADE (13), DODW_STICKGRENADE (14), DODW_MILLS_BOMB (36)
static cell AMX_NATIVE_CALL dodx_strip_grenade(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->pEdict || !pPlayer->pEdict->pvPrivateData)
		return 0;

	int grenadeType = params[2];

	// Clear all ammo slots for this grenade type
	switch (grenadeType)
	{
		case 13: // DODW_HANDGRENADE
		case 36: // DODW_MILLS_BOMB
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_1) = 0;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_2) = 0;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_HANDGRENADE_3) = 0;
			break;

		case 14: // DODW_STICKGRENADE
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_1) = 0;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_2) = 0;
			*((int*)pPlayer->pEdict->pvPrivateData + PDOFFSET_AMMO_STICKGRENADE_3) = 0;
			break;

		default:
			MF_LogError(amx, AMX_ERR_NATIVE, "dodx_strip_grenade: invalid grenade type %d", grenadeType);
			return 0;
	}

	return 1;
}

// dodx_debug_dump_ammo(id)
// Debug function to dump potential ammo offset values
// Scans for values 1-10 which could be grenade/ammo counts
static cell AMX_NATIVE_CALL dodx_debug_dump_ammo(AMX *amx, cell *params)
{
	int index = params[1];
	CHECK_PLAYER(index);

	CPlayer* pPlayer = GET_PLAYER_POINTER_I(index);
	if (!pPlayer->pEdict || !pPlayer->pEdict->pvPrivateData)
		return 0;

	int* pData = (int*)pPlayer->pEdict->pvPrivateData;

#if defined(__linux__) || defined(__APPLE__)
	// Show current offset adjustment
	MF_Log("[DODX DEBUG] Player %d - Runtime offset adjustment: +%d (detected=%s)",
		index, g_iLinuxPdataOffsetAdjust, g_bPdataOffsetDetected ? "yes" : "no");
	MF_Log("[DODX DEBUG] Expected offsets: HG1=%d HG2=%d HG3=%d SG1=%d",
		PDOFFSET_AMMO_HANDGRENADE_1, PDOFFSET_AMMO_HANDGRENADE_2,
		PDOFFSET_AMMO_HANDGRENADE_3, PDOFFSET_AMMO_STICKGRENADE_1);
#endif

	// Scan within safe bounds of DoD player private data (~175 ints / 700 bytes)
	MF_Log("[DODX DEBUG] Player %d - Scanning for values 1-10 (grenade counts):", index);

	// Scan offsets 0-175 (700 bytes) - safe range for DoD player private data
	for (int i = 0; i <= 175; i++) {
		int val = pData[i];
		if (val >= 1 && val <= 10) {
			const char* marker = "";
			if (i == PDOFFSET_AMMO_HANDGRENADE_1) marker = " <-- HANDGRENADE_1 (expected)";
			else if (i == PDOFFSET_AMMO_HANDGRENADE_2) marker = " <-- HANDGRENADE_2 (expected)";
			else if (i == PDOFFSET_AMMO_HANDGRENADE_3) marker = " <-- HANDGRENADE_3 (expected)";
			else if (i == PDOFFSET_AMMO_STICKGRENADE_1) marker = " <-- STICKGRENADE_1 (expected)";
			MF_Log("[DODX DEBUG]   [%d] = %d%s", i, val, marker);
		}
	}

	return 1;
}

AMX_NATIVE_INFO base_Natives[] =
{
	{ "dod_wpnlog_to_name", wpnlog_to_name },
	{ "dod_wpnlog_to_id", wpnlog_to_id },

	{ "dod_get_team_score", get_team_score },
	{ "dod_get_user_score", get_user_score },
	{ "dod_get_user_class", get_user_class },
	{ "dod_get_user_weapon", get_user_weapon },
	
	{ "dod_weapon_type", dod_weapon_type },

	{ "dod_get_map_info", get_map_info },
	{ "dod_user_kill", user_kill },
	{ "dod_get_pronestate", get_user_pronestate },

	{ "xmod_get_wpnname", get_weapon_name },
	{ "xmod_get_wpnlogname", get_weapon_logname },
	{ "xmod_is_melee_wpn", is_melee },
	{ "xmod_get_maxweapons", get_maxweapons },
	{ "xmod_get_stats_size", get_stats_size },
	{ "xmod_is_custom_wpn", is_custom },
  
	{ "register_statsfwd",register_forward },

	// Custom Weapon Support
	{ "custom_weapon_add", register_cwpn }, // name,melee,logname
	{ "custom_weapon_dmg", cwpn_dmg },
	{ "custom_weapon_shot", cwpn_shot },

	//****************************************

	// KTP: Disabled - use core AMXX get_user_team to avoid crash in extension mode
	// { "get_user_team", get_user_team },
	{ "get_weaponname", get_weapon_name },
	{ "get_user_weapon", get_user_weapon },
	{ "dod_get_user_team", dod_get_user_team },
	{ "dod_get_wpnname", get_weapon_name },
	{ "dod_get_wpnlogname", get_weapon_logname },
	{ "dod_is_melee", is_melee },

	{"dod_set_model",		dod_set_model},
	{"dod_set_body_number",	dod_set_body},
	{"dod_clear_model",		dod_clear_model},
	{"dod_set_weaponlist",	dod_weaponlist},

	// KTP: Scoreboard team name (extension mode compatible)
	{"dodx_set_pl_teamname", dodx_set_pl_teamname},

	// KTP: Gamerules score modification (scoreboard scores)
	{"dodx_set_team_score", dodx_set_team_score},
	{"dodx_get_team_score", dodx_get_team_score},
	{"dodx_has_gamerules", dodx_has_gamerules},
	{"dodx_broadcast_team_score", dodx_broadcast_team_score},

	// KTP: Custom scoreboard team names
	{"dodx_set_scoreboard_team_name", dodx_set_scoreboard_team_name},

	// KTP: Grenade ammo manipulation (extension mode compatible)
	{"dodx_set_grenade_ammo", dodx_set_grenade_ammo},
	{"dodx_get_grenade_ammo", dodx_get_grenade_ammo},

	// KTP: Noclip control (extension mode compatible)
	{"dodx_set_user_noclip", dodx_set_user_noclip},
	{"dodx_send_ammox", dodx_send_ammox},

	// KTP: Give grenade weapon (for practice mode infinite grenades)
	{"dodx_give_grenade", dodx_give_grenade},
	{"dodx_strip_grenade", dodx_strip_grenade},
	{"dodx_debug_dump_ammo", dodx_debug_dump_ammo},

	// KTP: Player class/team/position manipulation (hostname broadcast state restoration)
	{"dodx_set_user_class", dodx_set_user_class},
	{"dodx_set_user_team", dodx_set_user_team},
	{"dodx_get_user_origin", dodx_get_user_origin},
	{"dodx_set_user_origin", dodx_set_user_origin},
	{"dodx_get_user_angles", dodx_get_user_angles},
	{"dodx_set_user_angles", dodx_set_user_angles},

	///*******************
	{ NULL, NULL }
};
