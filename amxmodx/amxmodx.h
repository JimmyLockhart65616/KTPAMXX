// vim: set ts=4 sw=4 tw=99 noet:
//
// KTP AMX, based on AMX Mod X by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team, 2025 KTP.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#ifndef AMXMODX_H
#define AMXMODX_H

#if defined PLATFORM_POSIX
#include <unistd.h>
#include <stdlib.h>
#include "sclinux.h"
#endif
#include <ctype.h>				//tolower, etc
#include "string.h"
#include <extdll.h>
#ifdef USE_METAMOD
#include <meta_api.h>
#else
// Stub definitions for extension mode (no Metamod)
// Maximum registered user messages
#ifndef MAX_REG_MSGS
#define MAX_REG_MSGS 256
#endif

// Plugin load time (Metamod enum stub)
typedef enum {
	PT_NEVER = 0,
	PT_STARTUP,
	PT_CHANGELEVEL,
	PT_ANYTIME,
	PT_ANYPAUSE,
} PLUG_LOADTIME;

// Plugin unload reason
typedef enum {
	PNL_NULL = 0,
	PNL_INI_DELETED,
	PNL_FILE_NEWER,
	PNL_COMMAND,
	PNL_CMD_FORCED,
	PNL_DELAYED,
	PNL_PLUGIN,
	PNL_PLG_FORCED,
	PNL_RELOAD,
} PL_UNLOAD_REASON;

// HUD text message parameters
typedef struct hudtextparms_s {
	float x;
	float y;
	int effect;
	unsigned char r1, g1, b1, a1;
	unsigned char r2, g2, b2, a2;
	float fadeinTime;
	float fadeoutTime;
	float holdTime;
	float fxTime;
	int channel;
} hudtextparms_t;

// Engine functions extern
extern enginefuncs_t g_engfuncs;
#ifndef INDEXENT
#define INDEXENT(iEdictNum) (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum)
#endif
#ifndef VARS
#define VARS(pent) (&(pent)->v)
#endif
#ifndef IS_DEDICATED_SERVER
#define IS_DEDICATED_SERVER (*g_engfuncs.pfnIsDedicatedServer)
#endif
#ifndef SERVER_COMMAND
#define SERVER_COMMAND (*g_engfuncs.pfnServerCommand)
#endif
#ifndef STRING
#define STRING(offset) (const char *)(gpGlobals->pStringBase + (unsigned int)(offset))
#endif
// Plugin ID and request ID macros (Metamod stubs)
#ifndef PLID
#define PLID 1  // Extension mode doesn't need real plugin ID
#endif
#ifndef MAKE_REQUESTID
#define MAKE_REQUESTID(plid) (((plid) << 12) | 0x800)
#endif
#ifndef QUERY_CLIENT_CVAR_VALUE2
#define QUERY_CLIENT_CVAR_VALUE2 (*g_engfuncs.pfnQueryClientCvarValue2)
#endif
#ifndef FNullEnt
#define FNullEnt(pent) ((pent) == nullptr || (pent)->free || ENTOFFSET(pent) == 0)
#endif
#ifndef ENTOFFSET
#define ENTOFFSET(pent) ((int)((char*)(pent) - (char*)INDEXENT(0)))
#endif
// Game DLL function wrappers (Metamod stubs)
extern DLL_FUNCTIONS gEntityInterface;
#ifndef MDLL_Spawn
#define MDLL_Spawn gEntityInterface.pfnSpawn
#endif

// Engine cvar macros
#ifndef CVAR_GET_POINTER
#define CVAR_GET_POINTER(name) (*g_engfuncs.pfnCVarGetPointer)(name)
#endif
#ifndef CVAR_REGISTER
#define CVAR_REGISTER(x) (*g_engfuncs.pfnCVarRegister)(x)
#endif
#ifndef CVAR_SET_STRING
#define CVAR_SET_STRING(name, value) (*g_engfuncs.pfnCVarSetString)(name, value)
#endif
#ifndef CVAR_SET_FLOAT
#define CVAR_SET_FLOAT(name, value) (*g_engfuncs.pfnCVarSetFloat)(name, value)
#endif
#ifndef CVAR_GET_FLOAT
#define CVAR_GET_FLOAT(name) (*g_engfuncs.pfnCVarGetFloat)(name)
#endif
#ifndef CVAR_GET_STRING
#define CVAR_GET_STRING(name) (*g_engfuncs.pfnCVarGetString)(name)
#endif

// Engine command macros
#ifndef CMD_ARGC
#define CMD_ARGC (*g_engfuncs.pfnCmd_Argc)
#endif
#ifndef CMD_ARGV
#define CMD_ARGV (*g_engfuncs.pfnCmd_Argv)
#endif
#ifndef CMD_ARGS
#define CMD_ARGS (*g_engfuncs.pfnCmd_Args)
#endif
#ifndef REG_SVR_COMMAND
#define REG_SVR_COMMAND (*g_engfuncs.pfnAddServerCommand)
#endif
#ifndef SERVER_PRINT
#define SERVER_PRINT (*g_engfuncs.pfnServerPrint)
#endif

// Entity macros
#ifndef REMOVE_ENTITY
#define REMOVE_ENTITY (*g_engfuncs.pfnRemoveEntity)
#endif
#ifndef CREATE_NAMED_ENTITY
#define CREATE_NAMED_ENTITY (*g_engfuncs.pfnCreateNamedEntity)
#endif
#ifndef MAKE_STRING
#define MAKE_STRING(str) ((uint64)(str) - (uint64)(STRING(0)))
#endif
#ifndef ALLOC_STRING
#define ALLOC_STRING (*g_engfuncs.pfnAllocString)
#endif
#ifndef PRECACHE_MODEL
#define PRECACHE_MODEL (*g_engfuncs.pfnPrecacheModel)
#endif
#ifndef PRECACHE_SOUND
#define PRECACHE_SOUND (*g_engfuncs.pfnPrecacheSound)
#endif
#ifndef MODEL_INDEX
#define MODEL_INDEX (*g_engfuncs.pfnModelIndex)
#endif
#ifndef MODEL_FRAMES
#define MODEL_FRAMES (*g_engfuncs.pfnModelFrames)
#endif

// Misc engine macros
#ifndef GET_GAME_DIR
#define GET_GAME_DIR (*g_engfuncs.pfnGetGameDir)
#endif
#ifndef ALERT
#define ALERT (*g_engfuncs.pfnAlertMessage)
#endif
#ifndef ENGINE_FPRINTF
#define ENGINE_FPRINTF (*g_engfuncs.pfnEngineFprintf)
#endif
#ifndef GET_INFO_KEY_BUFFER
#define GET_INFO_KEY_BUFFER (*g_engfuncs.pfnGetInfoKeyBuffer)
#endif
#ifndef INFO_KEY_VALUE
#define INFO_KEY_VALUE (*g_engfuncs.pfnInfoKeyValue)
#endif
#ifndef SET_KEY_VALUE
#define SET_KEY_VALUE (*g_engfuncs.pfnSetKeyValue)
#endif
#ifndef SET_CLIENT_KEY_VALUE
#define SET_CLIENT_KEY_VALUE (*g_engfuncs.pfnSetClientKeyValue)
#endif
// Localinfo macros (server info buffer accessed via NULL)
#ifndef SET_LOCALINFO
#define SET_LOCALINFO(key, value) SET_KEY_VALUE(GET_INFO_KEY_BUFFER(NULL), (char*)(key), (char*)(value))
#endif
#ifndef GET_LOCALINFO
#define GET_LOCALINFO(key) INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(NULL), (key))
#endif
#ifndef ENTITY_KEYVALUE
#define ENTITY_KEYVALUE(e, key) INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(e), (key))
#endif
#ifndef GETPLAYERUSERID
#define GETPLAYERUSERID (*g_engfuncs.pfnGetPlayerUserId)
#endif
// MESSAGE_BEGIN macro - handle 3 or 4 arguments (4th is entity, defaults to NULL)
#ifndef MESSAGE_BEGIN
#define MESSAGE_BEGIN_3(dest, type, origin) (*g_engfuncs.pfnMessageBegin)(dest, type, origin, NULL)
#define MESSAGE_BEGIN_4(dest, type, origin, ent) (*g_engfuncs.pfnMessageBegin)(dest, type, origin, ent)
#define MESSAGE_BEGIN_GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define MESSAGE_BEGIN(...) MESSAGE_BEGIN_GET_MACRO(__VA_ARGS__, MESSAGE_BEGIN_4, MESSAGE_BEGIN_3)(__VA_ARGS__)
#endif
#ifndef MESSAGE_END
#define MESSAGE_END (*g_engfuncs.pfnMessageEnd)
#endif
#ifndef WRITE_BYTE
#define WRITE_BYTE (*g_engfuncs.pfnWriteByte)
#endif
#ifndef WRITE_CHAR
#define WRITE_CHAR (*g_engfuncs.pfnWriteChar)
#endif
#ifndef WRITE_SHORT
#define WRITE_SHORT (*g_engfuncs.pfnWriteShort)
#endif
#ifndef WRITE_LONG
#define WRITE_LONG (*g_engfuncs.pfnWriteLong)
#endif
#ifndef WRITE_ANGLE
#define WRITE_ANGLE (*g_engfuncs.pfnWriteAngle)
#endif
#ifndef WRITE_COORD
#define WRITE_COORD (*g_engfuncs.pfnWriteCoord)
#endif
#ifndef WRITE_STRING
#define WRITE_STRING (*g_engfuncs.pfnWriteString)
#endif
#ifndef WRITE_ENTITY
#define WRITE_ENTITY (*g_engfuncs.pfnWriteEntity)
#endif
#ifndef GET_USER_MSG_ID
#define GET_USER_MSG_ID(pfn, name, sz) (*g_engfuncs.pfnGetUserMsgID)(pfn, name, sz)
#endif
#ifndef GET_USER_MSG_NAME
#define GET_USER_MSG_NAME (*g_engfuncs.pfnGetUserMsgName)
#endif
#ifndef REG_USER_MSG
#define REG_USER_MSG (*g_engfuncs.pfnRegUserMsg)
#endif
#ifndef FIND_ENTITY_BY_STRING
#define FIND_ENTITY_BY_STRING (*g_engfuncs.pfnFindEntityByString)
#endif
#ifndef FIND_ENTITY_IN_SPHERE
#define FIND_ENTITY_IN_SPHERE (*g_engfuncs.pfnFindEntityInSphere)
#endif
#ifndef SET_SIZE
#define SET_SIZE (*g_engfuncs.pfnSetSize)
#endif
#ifndef SET_ORIGIN
#define SET_ORIGIN (*g_engfuncs.pfnSetOrigin)
#endif
#ifndef SET_MODEL
#define SET_MODEL (*g_engfuncs.pfnSetModel)
#endif
#ifndef DROP_TO_FLOOR
#define DROP_TO_FLOOR (*g_engfuncs.pfnDropToFloor)
#endif
#ifndef EMIT_SOUND_DYN
#define EMIT_SOUND_DYN (*g_engfuncs.pfnEmitSound)
#endif
#ifndef CLIENT_PRINTF
#define CLIENT_PRINTF (*g_engfuncs.pfnClientPrintf)
#endif
#ifndef POINT_CONTENTS
#define POINT_CONTENTS (*g_engfuncs.pfnPointContents)
#endif
#ifndef TRACE_LINE
#define TRACE_LINE (*g_engfuncs.pfnTraceLine)
#endif
#ifndef TRACE_HULL
#define TRACE_HULL (*g_engfuncs.pfnTraceHull)
#endif
#ifndef TRACE_MODEL
#define TRACE_MODEL (*g_engfuncs.pfnTraceModel)
#endif
#ifndef NUMBER_OF_ENTITIES
#define NUMBER_OF_ENTITIES (*g_engfuncs.pfnNumberOfEntities)
#endif
#ifndef GET_BONE_POSITION
#define GET_BONE_POSITION (*g_engfuncs.pfnGetBonePosition)
#endif
#ifndef GET_ATTACHMENT
#define GET_ATTACHMENT (*g_engfuncs.pfnGetAttachment)
#endif
#ifndef SET_VIEW
#define SET_VIEW (*g_engfuncs.pfnSetView)
#endif
#ifndef SET_CROSSHAIRANGLE
#define SET_CROSSHAIRANGLE (*g_engfuncs.pfnCrosshairAngle)
#endif
#ifndef CHANGE_LEVEL
#define CHANGE_LEVEL (*g_engfuncs.pfnChangeLevel)
#endif
#ifndef GET_AIM_VECTOR
#define GET_AIM_VECTOR (*g_engfuncs.pfnGetAimVector)
#endif
#ifndef FREE_ENTITY_PRIVATE_DATA
#define FREE_ENTITY_PRIVATE_DATA (*g_engfuncs.pfnFreeEntPrivateData)
#endif
#ifndef PLAYER_RUN_MOVE
#define PLAYER_RUN_MOVE (*g_engfuncs.pfnRunPlayerMove)
#endif
#ifndef TIME
#define TIME (*g_engfuncs.pfnTime)
#endif
#ifndef RANDOM_FLOAT
#define RANDOM_FLOAT (*g_engfuncs.pfnRandomFloat)
#endif
#ifndef RANDOM_LONG
#define RANDOM_LONG (*g_engfuncs.pfnRandomLong)
#endif
#ifndef CRC32_INIT
#define CRC32_INIT (*g_engfuncs.pfnCRC32_Init)
#endif
#ifndef CRC32_PROCESS_BUFFER
#define CRC32_PROCESS_BUFFER (*g_engfuncs.pfnCRC32_ProcessBuffer)
#endif
#ifndef CRC32_PROCESS_BYTE
#define CRC32_PROCESS_BYTE (*g_engfuncs.pfnCRC32_ProcessByte)
#endif
#ifndef CRC32_FINAL
#define CRC32_FINAL (*g_engfuncs.pfnCRC32_Final)
#endif
#ifndef MAKE_VECTORS
#define MAKE_VECTORS (*g_engfuncs.pfnMakeVectors)
#endif
#ifndef VEC_TO_ANGLES
#define VEC_TO_ANGLES (*g_engfuncs.pfnVecToAngles)
#endif
#ifndef ANGLE_VECTORS
#define ANGLE_VECTORS (*g_engfuncs.pfnAngleVectors)
#endif

// Dynamic library handle
#ifdef _WIN32
typedef HINSTANCE DLHANDLE;
#else
typedef void* DLHANDLE;
#endif

// Metamod result enum stubs
typedef enum {
	MRES_UNSET = 0,
	MRES_IGNORED,
	MRES_HANDLED,
	MRES_OVERRIDE,
	MRES_SUPERCEDE,
} META_RES;

// RETURN_META macro - in extension mode, just return from the function
#define RETURN_META(result) return
#define RETURN_META_VALUE(result, value) return (value)

// Entity index macros
#ifndef ENTINDEX
#define ENTINDEX(pent) ((pent) ? ((int)((char*)(pent) - (char*)INDEXENT(0)) / sizeof(edict_t)) : 0)
#endif

// GET_USER_MSG_NAME - in extension mode, Metamod's version with PLID isn't available
// Use direct engine call instead (pfnGetUserMsgName doesn't exist in HLSDK, so return NULL)
#undef GET_USER_MSG_NAME
#define GET_USER_MSG_NAME(plid, msgid, size) (NULL)

#endif // USE_METAMOD

#ifdef _MSC_VER
	// MSVC8 - replace POSIX functions with ISO C++ conformant ones as they are deprecated
	#if _MSC_VER >= 1400
		#define unlink _unlink
		#define mkdir _mkdir
		#define strdup _strdup
	#endif
#endif

#include "hashing.h"
#include "modules.h"
#include "CPlugin.h"
#include "CLibrarySys.h"
#include <auto-string.h>
#include <amtl/am-string.h>
#include <amtl/am-vector.h>
#include <amtl/am-inlinelist.h>
#include "CMisc.h"
#include "CVault.h"
#include "CModule.h"
#include "CTask.h"
#include "CLogEvent.h"
#include "CForward.h"
#include "CCmd.h"
#include "CEvent.h"
#include "CLang.h"
#include "fakemeta.h"
#include "amxxlog.h"
#include "CvarManager.h"
#include "CoreConfig.h"
#include "CFrameAction.h"
#include <amxmodx_version.h>
#include <HLTypeConversion.h>

#define AMXXLOG_Log g_log.Log
#define AMXXLOG_Error g_log.LogError

extern AMX_NATIVE_INFO core_Natives[];
extern AMX_NATIVE_INFO time_Natives[];
extern AMX_NATIVE_INFO power_Natives[];
extern AMX_NATIVE_INFO amxmodx_Natives[];
extern AMX_NATIVE_INFO file_Natives[];
extern AMX_NATIVE_INFO float_Natives[];
extern AMX_NATIVE_INFO string_Natives[];
extern AMX_NATIVE_INFO vault_Natives[];
extern AMX_NATIVE_INFO msg_Natives[];
extern AMX_NATIVE_INFO vector_Natives[];
extern AMX_NATIVE_INFO g_SortNatives[];
extern AMX_NATIVE_INFO g_DataStructNatives[];
extern AMX_NATIVE_INFO g_StackNatives[];
extern AMX_NATIVE_INFO g_TextParserNatives[];
extern AMX_NATIVE_INFO g_CvarNatives[];
extern AMX_NATIVE_INFO g_GameConfigNatives[];

#if defined PLATFORM_WINDOWS
#define DLLOAD(path) (DLHANDLE)LoadLibrary(path)
#define DLPROC(m, func) GetProcAddress(m, func)
#define DLFREE(m) FreeLibrary(m)
#else
#define DLLOAD(path) (DLHANDLE)dlopen(path, RTLD_NOW)
#define DLPROC(m, func) dlsym(m, func)
#define DLFREE(m) dlclose(m)
#endif

#if defined __GNUC__
	#include <stdint.h>
	typedef intptr_t _INT_PTR;
#else
	#if defined AMD64
		typedef __int64 _INT_PTR;
	#else
		typedef __int32 _INT_PTR;
	#endif
#endif

#if defined PLATFORM_WINDOWS
	typedef HINSTANCE DLHANDLE;
#else
	typedef void* DLHANDLE;
	#define INFINITE 0xFFFFFFFF
#endif

#ifndef GETPLAYERAUTHID
#define GETPLAYERAUTHID     (*g_engfuncs.pfnGetPlayerAuthId)
#endif
#define ANGLEVECTORS        (*g_engfuncs.pfnAngleVectors)
#define CLIENT_PRINT        (*g_engfuncs.pfnClientPrintf)
#define CVAR_DIRECTSET      (*g_engfuncs.pfnCvar_DirectSet)
#define GETCLIENTLISTENING  (*g_engfuncs.pfnVoice_GetClientListening)
#define RUNPLAYERMOVE       (*g_engfuncs.pfnRunPlayerMove)
#define SETCLIENTLISTENING  (*g_engfuncs.pfnVoice_SetClientListening)
#define SETCLIENTMAXSPEED   (*g_engfuncs.pfnSetClientMaxspeed)

// KTP: Game DLL function wrappers that work in both Metamod and extension mode
// These use g_pGameEntityInterface which is set to either:
// - gpGamedllFuncs->dllapi_table (Metamod mode)
// - RehldsFuncs->GetEntityInterface() (ReHLDS extension mode)
#define KTPAMX_ClientKill(pEntity)          (g_pGameEntityInterface ? g_pGameEntityInterface->pfnClientKill(pEntity) : (void)0)
#define KTPAMX_ClientCommand(pEntity)       (g_pGameEntityInterface ? g_pGameEntityInterface->pfnClientCommand(pEntity) : (void)0)
#define KTPAMX_ClientConnect(pE,n,a,r)      (g_pGameEntityInterface ? g_pGameEntityInterface->pfnClientConnect(pE,n,a,r) : 0)
#define KTPAMX_ClientDisconnect(pEntity)    (g_pGameEntityInterface ? g_pGameEntityInterface->pfnClientDisconnect(pEntity) : (void)0)
#define KTPAMX_ClientPutInServer(pEntity)   (g_pGameEntityInterface ? g_pGameEntityInterface->pfnClientPutInServer(pEntity) : (void)0)
#define KTPAMX_Spawn(pEntity)               (g_pGameEntityInterface ? g_pGameEntityInterface->pfnSpawn(pEntity) : 0)

#define MAX_BUFFER_LENGTH 16384

char* UTIL_SplitHudMessage(register const char *src);
int UTIL_ReadFlags(const char* c);

void UTIL_ClientPrint(edict_t *pEntity, int msg_dest, char *msg);
void UTIL_FakeClientCommand(edict_t *pEdict, const char *cmd, const char *arg1 = NULL, const char *arg2 = NULL, bool fwd = false);
void UTIL_GetFlags(char* flags, int flag);
void UTIL_HudMessage(edict_t *pEntity, const hudtextparms_t &textparms, const char *pMessage);
void UTIL_DHudMessage(edict_t *pEntity, const hudtextparms_t &textparms, const char *pMessage, unsigned int length);
void UTIL_IntToString(int value, char *output);
void UTIL_ShowMOTD(edict_t *client, char *motd, int mlen, const char *name);
void UTIL_ShowMenu(edict_t* pEntity, int slots, int time, char *menu, int mlen);
void UTIL_ClientSayText(edict_t *pEntity, int sender, char *msg);
void UTIL_TeamInfo(edict_t *pEntity, int playerIndex, const char *pszTeamName);

template <typename D> int UTIL_CheckValidChar(D *c);
template <typename D, typename S> unsigned int strncopy(D *dest, const S *src, size_t count);
unsigned int UTIL_GetUTF8CharBytes(const char *stream);
size_t UTIL_ReplaceAll(char *subject, size_t maxlength, const char *search, const char *replace, bool caseSensitive);
size_t UTIL_ReplaceAll(char *subject, size_t maxlength, const char *search, size_t searchLen, const char *replace, size_t replaceLen, bool caseSensitive);
char *UTIL_ReplaceEx(char *subject, size_t maxLen, const char *search, size_t searchLen, const char *replace, size_t replaceLen, bool caseSensitive);
void UTIL_TrimLeft(char *buffer);
void UTIL_TrimRight(char *buffer);

char* utf8stristr(const char *string1, const char *string2);
int utf8strncasecmp(const char *string1, const char *string2, size_t n);
int utf8strcasecmp(const char *string1, const char *string2);

#define GET_PLAYER_POINTER(e)   (&g_players[ENTINDEX(e)])
//#define GET_PLAYER_POINTER(e)   (&g_players[(((int)e-g_edict_point)/sizeof(edict_t))])
#define GET_PLAYER_POINTER_I(i) (&g_players[i])

struct WeaponsVault
{
	ke::AString fullName;
	short int iId;
	short int ammoSlot;
};

struct fakecmd_t
{
	char args[256];
	const char *argv[3];
	int argc;
	bool fake;
	bool notify; // notify to plugins.
};

extern CLog g_log;
extern CPluginMngr g_plugins;
extern CTaskMngr g_tasksMngr;
extern CFrameActionMngr g_frameActionMngr;
extern CPlayer g_players[33];
extern CPlayer* mPlayer;
extern CmdMngr g_commands;
extern ke::Vector<ke::AutoPtr<ForceObject>> g_forcemodels;
extern ke::Vector<ke::AutoPtr<ForceObject>> g_forcesounds;
extern ke::Vector<ke::AutoPtr<ForceObject>> g_forcegeneric;
extern ke::Vector<ke::AutoPtr<CPlayer *>> g_auth;
extern ke::InlineList<CModule> g_modules;
extern ke::InlineList<CScript> g_loadedscripts;
extern EventsMngr g_events;
extern Grenades g_grenades;
extern LogEventsMngr g_logevents;
extern CLangMngr g_langMngr;
extern ke::AString g_log_dir;
extern ke::AString g_mod_name;
extern TeamIds g_teamsIds;
extern Vault g_vault;
extern CForwardMngr g_forwards;
extern WeaponsVault g_weaponsData[MAX_WEAPONS];
extern XVars g_xvars;
extern bool g_bmod_cstrike;
extern bool g_bmod_dod;
extern bool g_bmod_dmc;
extern bool g_bmod_ricochet;
extern bool g_bmod_valve;
extern bool g_bmod_gearbox;
extern bool g_official_mod;
extern bool g_dontprecache;
extern int g_srvindex;
extern cvar_t* amxmodx_version;
extern cvar_t* amxmodx_debug;
extern cvar_t* amxmodx_language;
extern cvar_t* amxmodx_perflog;
extern cvar_t* hostname;
extern cvar_t* mp_timelimit;
extern fakecmd_t g_fakecmd;
extern float g_game_restarting;
extern float g_game_timeleft;
extern float g_task_time;
extern float g_auth_time;
extern bool g_NewDLL_Available;
extern bool g_bRunningWithMetamod;        // KTP: True when running with Metamod
extern bool g_bRehldsExtensionInit;       // KTP: True when initialized as ReHLDS extension
extern DLL_FUNCTIONS *g_pGameEntityInterface;  // KTP: Game DLL functions - works in both modes
extern hudtextparms_t g_hudset;
//extern int g_edict_point;
extern int g_players_num;
extern int mPlayerIndex;
extern int mState;
extern void (*endfunction)(void*);
extern void (*function)(void*);

typedef void (*funEventCall)(void*);
extern funEventCall modMsgsEnd[MAX_REG_MSGS];
extern funEventCall modMsgs[MAX_REG_MSGS];

extern int gmsgAmmoPickup;
extern int gmsgAmmoX;
extern int gmsgBattery;
extern int gmsgCurWeapon;
extern int gmsgDamage;
extern int gmsgDeathMsg;
extern int gmsgHealth;
extern int gmsgMOTD;
extern int gmsgScoreInfo;
extern int gmsgSendAudio;
extern int gmsgServerName;
extern int gmsgShowMenu;
extern int gmsgTeamInfo;
extern int gmsgTextMsg;
extern int gmsgVGUIMenu;
extern int gmsgWeapPickup;
extern int gmsgWeaponList;
extern int gmsgintermission;
extern int gmsgResetHUD;
extern int gmsgRoundTime;
extern int gmsgSayText;
extern int gmsgInitHUD;

void Client_AmmoPickup(void*);
void Client_AmmoX(void*);
void Client_CurWeapon(void*);
void Client_ScoreInfo(void*);
void Client_ShowMenu(void*);
void Client_TeamInfo(void*);
void Client_TextMsg(void*);
void Client_VGUIMenu(void*);
void Client_WeaponList(void*);
void Client_DamageEnd(void*);
void Client_DeathMsg(void*);
void Client_InitHUDEnd(void*);

void amx_command();
void plugin_srvcmd();

const char* stristr(const char* a, const char* b);
char *strptime(const char *buf, const char *fmt, struct tm *tm, short addthem);

int loadModules(const char* filename, PLUG_LOADTIME now);
void detachModules();
void detachReloadModules();

// Count modules
enum CountModulesMode
{
	CountModules_Running = 0,
	CountModules_All,
	CountModules_Stopped
};

int countModules(CountModulesMode mode);
void modules_callPluginsLoaded();
void modules_callPluginsUnloaded();
void modules_callPluginsUnloading();

cell* get_amxaddr(AMX *amx, cell amx_addr);
char* build_pathname(const char *fmt, ...);
char* build_pathname_r(char *buffer, size_t maxlen, const char *fmt, ...);
char* format_amxstring(AMX *amx, cell *params, int parm, int& len);
AMX* get_amxscript(int, void**, const char**);
const char* get_amxscriptname(AMX* amx);
char* get_amxstring(AMX *amx, cell amx_addr, int id, int& len);
char* get_amxstring_null(AMX *amx, cell amx_addr, int id, int& len);
cell* get_amxvector_null(AMX *amx, cell amx_addr);
extern "C" size_t get_amxstring_r(AMX *amx, cell amx_addr, char *destination, int maxlen);

int amxstring_len(cell* cstr);
int load_amxscript(AMX* amx, void** program, const char* path, char error[64], int debug);
int load_amxscript_ex(AMX* amx, void** program, const char* path, char *error, size_t maxLength, int debug);
int set_amxnatives(AMX* amx, char error[64]);
int set_amxstring(AMX *amx, cell amx_addr, const char *source, int max);
int set_amxstring_simple(cell *dest, const char *source, int max);
template <typename T> int set_amxstring_utf8(AMX *amx, cell amx_addr, const T *source, size_t sourcelen, size_t maxlen);
int set_amxstring_utf8_char(AMX *amx, cell amx_addr, const char *source, size_t sourcelen, size_t maxlen);
int set_amxstring_utf8_cell(AMX *amx, cell amx_addr, const cell *source, size_t sourcelen, size_t maxlen);
int unload_amxscript(AMX* amx, void** program);

void copy_amxmemory(cell* dest, cell* src, int len);
void get_modname(char*);
void print_srvconsole(const char *fmt, ...);
void report_error(int code, const char* fmt, ...);
// get_localinfo
const char* get_localinfo(const char* name, const char* def);
extern "C" void LogError(AMX *amx, int err, const char *fmt, ...);

enum ModuleCallReason
{
	ModuleCall_NotCalled = 0,					// nothing
	ModuleCall_Query,							// in Query func
	ModuleCall_Attach,							// in Attach func
	ModuleCall_Detach,							// in Detach func
};

extern ModuleCallReason g_ModuleCallReason;		// modules.cpp
extern CModule *g_CurrentlyCalledModule;		// modules.cpp
extern const char *g_LastRequestedFunc;			// modules.cpp

void Module_CacheFunctions();
void Module_UncacheFunctions();

void *Module_ReqFnptr(const char *funcName);	// modules.cpp

// KTP: Module frame callback for modules that need per-frame processing (like cURL async)
// This replaces Metamod's pfnStartFrame callback for modules in extension mode
typedef void (*MODULEFRAMEFUNC)(void);

// KTP: Module frame callback registration (modules.cpp)
void MNF_RegModuleFrameFunc(MODULEFRAMEFUNC func);
void MNF_UnregModuleFrameFunc(MODULEFRAMEFUNC func);
void Module_ExecuteFrameCallbacks();

// standard forwards
// defined in meta_api.cpp
extern int FF_ClientCommand;
extern int FF_ClientConnect;
extern int FF_ClientDisconnect;
extern int FF_ClientInfoChanged;
extern int FF_ClientPutInServer;
extern int FF_PluginInit;
extern int FF_PluginCfg;
extern int FF_PluginPrecache;
extern int FF_PluginLog;
extern int FF_PluginEnd;
extern int FF_InconsistentFile;
extern int FF_ClientAuthorized;
extern int FF_ChangeLevel;
extern int FF_ClientConnectEx;

extern bool g_coloredmenus;

typedef void (*AUTHORIZEFUNC)(int player, const char *authstring);

#define MM_CVAR2_VERS	13

struct func_s
{
	void *pfn;
	const char *desc;
};

enum AdminProperty
{
	Admin_Auth = 0,
	Admin_Password,
	Admin_Access,
	Admin_Flags
};

enum PrintColor
{
	print_team_default = 0,
	print_team_grey =-1,
	print_team_red = -2,
	print_team_blue = -3,
};

extern enginefuncs_t *g_pEngTable;
extern HLTypeConversion TypeConversion;

#endif // AMXMODX_H
