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

#ifndef DODX_H
#define DODX_H

#include "amxxmodule.h"
#include "CMisc.h"
#include "CRank.h"
#include <IGameConfigs.h>

// KTP: First edict pointer for safe entity index calculation
// This avoids calling engine functions in extension mode hooks
extern edict_t* g_pFirstEdict;

// KTP: Safe ENTINDEX that uses pointer arithmetic instead of engine function
// This is safe to call during ReHLDS hooks when pfnIndexOfEdict may crash
inline int ENTINDEX_SAFE(const edict_t *pEdict)
{
	if (!pEdict || !g_pFirstEdict)
		return 0;
	return static_cast<int>(pEdict - g_pFirstEdict);
}

// KTP: Use ENTINDEX_SAFE for GET_PLAYER_POINTER to avoid crashes in extension mode
#define GET_PLAYER_POINTER(e)   (&players[ENTINDEX_SAFE(e)])
#define GET_PLAYER_POINTER_I(i) (&players[i])

#ifndef GETPLAYERAUTHID
	#define GETPLAYERAUTHID		(*g_engfuncs.pfnGetPlayerAuthId)
#endif

extern AMX_NATIVE_INFO stats_Natives[];
extern AMX_NATIVE_INFO base_Natives[];
extern AMX_NATIVE_INFO pd_Natives[];
extern AMX_NATIVE_INFO cp_Natives[];

// Weapons grabbing by type
enum
{
	DODWT_PRIMARY = 0,
	DODWT_SECONDARY,
	DODWT_MELEE,
	DODWT_GRENADE, 
	DODWT_OTHER
};

// Model Sequences
enum
{
	DOD_SEQ_PRONE_IDLE = 15,
	DOD_SEQ_PRONE_FORWARD,
	DOD_SEQ_PRONE_DOWN,
	DOD_SEQ_PRONE_UP
};

// KTP: Player private data offsets for scoreboard team name
// Same offsets as dodfun module, but implemented here for extension mode compatibility
#if defined(__linux__) || defined(__APPLE__)
	#define STEAM_PDOFFSET_TEAMNAME (1400 + 5)  // Linux offset adjustment
	#define STEAM_PDOFFSET_SCORE    (476 + 5)   // Player score
	#define STEAM_PDOFFSET_DEATHS   (477 + 5)   // Player deaths
#else
	#define STEAM_PDOFFSET_TEAMNAME 1400        // Windows offset
	#define STEAM_PDOFFSET_SCORE    476         // Player score
	#define STEAM_PDOFFSET_DEATHS   477         // Player deaths
#endif

// KTP: Grenade ammo offsets (ported from dodfun for extension mode)
// Each grenade type has 3 offsets that need to be set together
//
// Linux offset adjustment varies by OS version:
//   Ubuntu 22.04 and older: +5
//   Ubuntu 24.04 and newer: +4
// Runtime detection is used to automatically determine the correct offset.
//
// Base offsets (without adjustment):
#define PDOFFSET_BASE_HANDGRENADE_1  59
#define PDOFFSET_BASE_HANDGRENADE_2  289
#define PDOFFSET_BASE_HANDGRENADE_3  321
#define PDOFFSET_BASE_STICKGRENADE_1 61
#define PDOFFSET_BASE_STICKGRENADE_2 291
#define PDOFFSET_BASE_STICKGRENADE_3 323

#if defined(__linux__) || defined(__APPLE__)
	// Runtime offset adjustment - detected at first player spawn
	// Default to +4 (Ubuntu 24.04), will be auto-detected
	// Can be forced via addons/ktpamx/configs/dodx.ini: pdata_offset = 4 or 5
	extern int g_iLinuxPdataOffsetAdjust;
	extern bool g_bPdataOffsetDetected;
	extern bool g_bPdataOffsetForced;

	// Phase 1: Write to both +4 and +5 offsets when offset is unknown
	void DODX_PdataWriteBoth(edict_t* pEdict, int grenadeType, int count);
	// Phase 2: Detect correct offset by scoring which has valid values
	void DODX_DetectPdataOffset(edict_t* pEdict);

	// Macros that use runtime offset adjustment
	#define PDOFFSET_AMMO_HANDGRENADE_1  (PDOFFSET_BASE_HANDGRENADE_1 + g_iLinuxPdataOffsetAdjust)
	#define PDOFFSET_AMMO_HANDGRENADE_2  (PDOFFSET_BASE_HANDGRENADE_2 + g_iLinuxPdataOffsetAdjust)
	#define PDOFFSET_AMMO_HANDGRENADE_3  (PDOFFSET_BASE_HANDGRENADE_3 + g_iLinuxPdataOffsetAdjust)
	#define PDOFFSET_AMMO_STICKGRENADE_1 (PDOFFSET_BASE_STICKGRENADE_1 + g_iLinuxPdataOffsetAdjust)
	#define PDOFFSET_AMMO_STICKGRENADE_2 (PDOFFSET_BASE_STICKGRENADE_2 + g_iLinuxPdataOffsetAdjust)
	#define PDOFFSET_AMMO_STICKGRENADE_3 (PDOFFSET_BASE_STICKGRENADE_3 + g_iLinuxPdataOffsetAdjust)
#else
	// Windows: no adjustment needed
	#define PDOFFSET_AMMO_HANDGRENADE_1  PDOFFSET_BASE_HANDGRENADE_1
	#define PDOFFSET_AMMO_HANDGRENADE_2  PDOFFSET_BASE_HANDGRENADE_2
	#define PDOFFSET_AMMO_HANDGRENADE_3  PDOFFSET_BASE_HANDGRENADE_3
	#define PDOFFSET_AMMO_STICKGRENADE_1 PDOFFSET_BASE_STICKGRENADE_1
	#define PDOFFSET_AMMO_STICKGRENADE_2 PDOFFSET_BASE_STICKGRENADE_2
	#define PDOFFSET_AMMO_STICKGRENADE_3 PDOFFSET_BASE_STICKGRENADE_3
#endif

// Weapons Structure
struct weapon_t 
{
	bool needcheck;
	bool melee;
	char logname[16];
	char name[32];
	int ammoSlot;
	int type;
};

struct weaponlist_s
{
	int grp;
	int bitfield;
	int clip;
	bool changeable;
};

// =============== KTP: Control Point Tracking (ported from dodfun) ===============

// DoD Control Point private data layout
struct pd_dcp {
	int iunk_0;
#if defined(_WIN32)
	int iunk_1; // windows only
#endif
	int iunk_2;	// pointer edict_t*
	int iunk_3;

	float origin_x;
	float origin_y;
	float origin_z; // 6

	float mins_x;
	float mins_y;
	float mins_z;

	float maxs_x;
	float maxs_y;
	float maxs_z;

	float angles_x;
	float angles_y;
	float angles_z; // 15

	int unknown_block1[19];
	int iunk_35; // pointer entvars_t*
	int iunk_36; // pointer entvars_t*
	int unknown_block2[52];
	int iunk_89; // pointer entvars_t*
#if defined (__linux__) || defined (__APPLE__)
	int iunk_extra1;
	int iunk_extra2;
	int iunk_extra3;
	int iunk_extra4;
#endif
	int owner; // 90
	int iunk_91;
	int iunk_92;
	int default_owner; // 93
	int flag_id;
	int pointvalue;
	int points_for_player;
	int points_for_team;
	float funk_98; // always 1.0
	float cap_time;
	char cap_message[256]; // 100
	int iunk_164;
	int iunk_165;
	char target_allies[256]; //  166
	char target_axis[256]; // 230
	char target_reset[256];
	char model_allies[256]; // 358
	char model_axis[256]; // 422
	char model_neutral[256]; // 486
	int model_body_allies; // 550
	int model_body_axis;
	int model_body_neutral;
	int icon_allies;
	int icon_axis;
	int icon_neutral;
	int can_touch; // flags : 1-allies can't, 256-axis can't , default 0 (all can)
	int iunk_557;
	int iunk_558;
	char pointgroup[256];
	int iunk_623;
	int iunk_624;
	int iunk_625;
};

#define GET_CP_PD( x ) (*(pd_dcp*)x->pvPrivateData)

// DoD Capture Area private data layout
struct pd_dca {
	int iunk_0;
	int iunk_1;
	int iunk_2;
#if defined(_WIN32)
	int iunk_3; // if def windows
#endif

	float origin_x;
	float origin_y;
	float origin_z; // 6

	float mins_x;
	float mins_y;
	float mins_z;

	float maxs_x;
	float maxs_y;
	float maxs_z;

	float angles_x;
	float angles_y;
	float angles_z; // 15

#if defined(_WIN32)
	int unknown_block_16[111];
#else
	int unknown_block_16[116]; // linux +5 more
#endif

	int time_to_cap; // 127
	int iunk_128;
	int allies_numcap; // 129
	int axis_numcap; // 130

	int iunk_131;
	int iunk_132;

	int can_cap; // 133 flags : 1-allies can , 256-axis can, default 257 (all can)

	int iunk_134;
	int iunk_135;

	char allies_endcap[256]; // 136
	char axis_endcap[256]; // 200
	char allies_startcap[256]; // 264
	char axis_startcap[256]; // 328
	char allies_breakcap[256]; // 392
	char axis_breakcap[256]; // 456
	int iunk_520;
	char hud_sprite[256]; // 521

	int unknown_block_585[65];

	char object_group[256]; // 650
	int iunk_714;
	int iunk_715;
	int iunk_716;
};

#define GET_CA_PD( x ) (*(pd_dca*)x->pvPrivateData)

// Control point info struct
typedef struct objinfo_s {
	// initobj
	edict_t* pEdict;
	int index;
	int default_owner;
	int visible;
	int icon_neutral;
	int icon_allies;
	int icon_axis;
	float origin_x;
	float origin_y;
	// setobj
	int owner;
	// control area
	int areaflags; // 0-need check , 1-no area , 2-found area
	edict_t* pAreaEdict;
} objinfo_t;

// Control point manager
class CObjective {
public:
	int count;
	objinfo_t obj[12];
	inline void Clear() { count = 0; memset(obj, 0, sizeof(obj)); }
	void SetKeyValue(int index, char *keyname, char *value);
	void InitObj(int dest = MSG_ALL, edict_t* ed = NULL);
	void SetObj(int index);
	void UpdateOwner(int index, int team);
	void Sort();
};

// Control point data keys
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

// Capture area data keys
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

// Macro to lazily find capture area entity for a control point
#define GET_CAPTURE_AREA(x) \
	if ( mObjects.obj[x].areaflags == 0 ){\
		mObjects.obj[x].areaflags = 1;\
		while ( (mObjects.obj[x].pAreaEdict = FindEntityByString(mObjects.obj[x].pAreaEdict,"target",STRING(mObjects.obj[x].pEdict->v.targetname))) )\
			if ( strcmp( STRING(mObjects.obj[x].pAreaEdict->v.classname),"dod_capture_area" )==0){\
				mObjects.obj[x].areaflags = 2;\
				break;\
			}\
	}\
	if ( mObjects.obj[x].areaflags == 1 )\
		return 0;

extern bool rankBots;
extern int mState;
extern int mDest;
extern int mCurWpnEnd;
extern int mPlayerIndex;

void Client_CurWeapon(void*);
void Client_CurWeapon_End(void*);
void Client_Health_End(void*);
void Client_ResetHUD_End(void*);
void Client_ObjScore(void*);
void Client_TeamScore(void*);
void Client_RoundState(void*);
void Client_AmmoX(void*);
void Client_AmmoShort(void*);
void Client_SetFOV(void*);
void Client_SetFOV_End(void*);
void Client_Object(void*);
void Client_Object_End(void*);
void Client_PStatus(void*);
void Client_InitObj(void*);
void Client_SetObj(void*);

typedef void (*funEventCall)(void*);

extern int AlliesScore;
extern int AxisScore;

extern int gmsgCurWeapon;
extern int gmsgCurWeaponEnd;
extern int gmsgHealth;
extern int gmsgResetHUD;
extern int gmsgObjScore;
extern int gmsgRoundState;
extern int gmsgTeamScore;
extern int gmsgScoreShort;
extern int gmsgPTeam;
extern int gmsgAmmoX;
extern int gmsgAmmoShort;
extern int gmsgSetFOV;
extern int gmsgSetFOV_End;
extern int gmsgObject;
extern int gmsgObject_End;
extern int gmsgPStatus;
extern int gmsgTeamInfo;  // KTP: For scoreboard team name refresh
extern int gmsgInitObj;   // KTP: CP tracking
extern int gmsgSetObj;    // KTP: CP tracking

extern int iFDamage;
extern int iFDeath;
extern int iFScore;
extern int iFSpawnForward;
extern int iFTeamForward;
extern int iFClassForward;
extern int iFScopeForward;
extern int iFProneForward;
extern int iFWpnPickupForward;
extern int iFCurWpnForward;
extern int iFGrenadeExplode;
extern int iFRocketExplode;
extern int iFObjectTouched;
extern int iFStaminaForward;
extern int iFFlushStats;  // KTP: Forward for stats flush notification
extern int iFDamagePre;   // KTP: Forward for damage modification (fires before client_damage)
extern int iFInitCP;      // KTP: Forward for CP init
extern int iFCPCaptured;  // KTP: Forward for CP ownership change
extern int iFScoreEvent;  // KTP: Forward for enriched score event with CP context

// KTP: Last CP capture tracking (for ObjScore correlation)
extern int g_lastCapturedCP;     // CP index from most recent SetObj (-1 = none)
extern float g_lastCapturedTime; // gpGlobals->time when last SetObj fired

// KTP: Match ID for HLStatsX integration
extern char g_szMatchId[64];
const char* DODX_GetMatchId();

extern cvar_t* dodstats_maxsize;
extern cvar_t* dodstats_rank;
extern cvar_t* dodstats_reset;
extern cvar_t* dodstats_rankbots;
extern cvar_t* dodstats_pause;

// KTP: Plugin-controlled stats pause (for round-freeze filtering)
extern bool g_bStatsPaused;

extern weapon_t weaponData[DODMAX_WEAPONS];
extern traceVault traceData[MAX_TRACE];

extern Grenades g_grenades;
extern RankSystem g_rank;
extern CPlayer players[33];
extern CPlayer* mPlayer;
extern CMapInfo g_map;
extern CObjective mObjects;

int get_weaponid(CPlayer* player);
bool ignoreBots (edict_t *pEnt, edict_t *pOther = NULL );
bool isModuleActive();
edict_t *FindEntityByString(edict_t *pentStart, const char *szKeyword, const char *szValue);
edict_t *FindEntityByClassname(edict_t *pentStart, const char *szName);
edict_t *FindEntityInSphere(edict_t *pentStart, edict_t *origin, float radius);

#define CHECK_ENTITY(x) \
	if (x < 0 || x > gpGlobals->maxEntities) { \
		MF_LogError(amx, AMX_ERR_NATIVE, "Entity out of range (%d)", x); \
		return 0; \
	} else { \
		if (x <= gpGlobals->maxClients) { \
			if (!MF_IsPlayerIngame(x)) { \
				MF_LogError(amx, AMX_ERR_NATIVE, "Invalid player %d (not in-game)", x); \
				return 0; \
			} \
		} else { \
			if (x != 0 && FNullEnt(INDEXENT(x))) { \
				MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity %d", x); \
				return 0; \
			} \
		} \
	}

// KTP: Use players[] array directly to avoid AMXX function calls that may crash in extension mode
#define CHECK_PLAYER(x) \
	if (x < 1 || x > gpGlobals->maxClients) { \
		MF_LogError(amx, AMX_ERR_NATIVE, "Player out of range (%d)", x); \
		return 0; \
	} else { \
		CPlayer* _pCheck = &players[x]; \
		if (!_pCheck->ingame || !_pCheck->pEdict || _pCheck->pEdict->free || FNullEnt(_pCheck->pEdict)) { \
			return 0; \
		} \
	}

#define CHECK_PLAYERRANGE(x) \
	if (x > gpGlobals->maxClients || x < 1) \
	{ \
		MF_LogError(amx, AMX_ERR_NATIVE, "Player out of range (%d)", x); \
		return 0; \
	}

#define CHECK_NONPLAYER(x) \
	if (x < 1 || x <= gpGlobals->maxClients || x > gpGlobals->maxEntities) { \
		MF_LogError(amx, AMX_ERR_NATIVE, "Non-player entity %d out of range", x); \
		return 0; \
	} else { \
		if (FNullEnt(INDEXENT(x))) { \
			MF_LogError(amx, AMX_ERR_NATIVE, "Invalid non-player entity %d", x); \
			return 0; \
		} \
	}

#define GETEDICT(n) \
	((n >= 1 && n <= gpGlobals->maxClients) ? MF_GetPlayerEdict(n) : INDEXENT(n))

// KTP: Gamerules access for scoreboard score modification
// Loaded from common.games gamedata - signature scan finds g_pGameRules
extern IGameConfig *g_pCommonConfig;
extern IGameConfig *g_pGamerulesConfig;
extern void **g_pGameRulesAddress;  // Pointer to g_pGameRules pointer
extern int g_iTeamScoreOffset;       // Offset of m_iTeamScores in CDoDTeamPlay (56)

// Check if gamerules is available for score modification
inline bool DODX_HasGameRules()
{
	return (g_pGameRulesAddress && *g_pGameRulesAddress);
}

#endif // DODX_H
