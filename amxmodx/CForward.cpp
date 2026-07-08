// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#include "amxmodx.h"
#include "debugger.h"
#include "binlog.h"

#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

// KTP: Bounded safe string scan for forward string params.
// Stale/UAF pointers often land at high-but-unmapped addresses (e.g.
// 0x3f145406) that SEGV inside libc strlen(). mincore() tells us whether a
// page is mapped, but a single-page check isn't enough: strlen can walk off
// the end of a mapped page into an unmapped neighbor. So probe every page
// the scan enters, and give up (treat as garbage) if no terminator shows up
// within a budget no legitimate forward string approaches.
// Returns the string length, or -1 if the pointer is NULL/low/unmapped or
// unterminated within budget. Coverage note: a freed-but-still-mapped heap
// page (glibc rarely munmaps small chunks) and PROT_NONE regions still pass
// mincore — those read stale bytes or SEGV; only unmapped-space wild
// pointers (the class seen in fleet cores) are caught here.
// On _WIN32 falls back to the old low-address check.
static int amxx_safe_string_length(const char *str)
{
	if (!str || reinterpret_cast<uintptr_t>(str) < 0x1000)
		return -1;
#ifndef _WIN32
	static const long page_size = sysconf(_SC_PAGESIZE);
	const int MAX_PROBE_PAGES = 8; // 32KB budget with 4KB pages
	uintptr_t p = reinterpret_cast<uintptr_t>(str);
	uintptr_t page = p & ~(page_size - 1);
	size_t len = 0;

	for (int pg = 0; pg < MAX_PROBE_PAGES; pg++)
	{
		unsigned char vec;
		if (mincore(reinterpret_cast<void*>(page), 1, &vec) != 0)
			return -1;
		size_t avail = (page + page_size) - p;
		size_t n = strnlen(reinterpret_cast<const char*>(p), avail);
		len += n;
		if (n < avail)
			return static_cast<int>(len);
		p = page + page_size;
		page = p;
	}
	return -1;
#else
	return static_cast<int>(strlen(str));
#endif
}

// KTP: Probe whether [ptr, ptr+len) is writable without risking a SEGV.
// The FP_STRINGEX write-back used to reuse the read check, but a readable
// page can still be read-only (e.g. a string literal passed through a
// mismatched param type) and the write faults. The kernel probes for us:
// write(pipe, addr, 1) EFAULTs if addr isn't readable, and read(pipe, addr, 1)
// EFAULTs if it isn't writable — round-tripping the byte through the pipe
// leaves memory unchanged on success. One probe per page touched.
// Falls back to "assume writable" (pre-existing behavior) if the pipe can't
// be created, and on _WIN32.
static bool amxx_is_range_writable(void *ptr, size_t len)
{
	if (!ptr || reinterpret_cast<uintptr_t>(ptr) < 0x1000)
		return false;
#ifndef _WIN32
	static int probe_fds[2] = { -1, -1 };
	static bool probe_init_failed = false;
	if (probe_fds[0] == -1)
	{
		if (probe_init_failed || pipe(probe_fds) != 0)
		{
			probe_fds[0] = probe_fds[1] = -1;
			probe_init_failed = true;
			return true;
		}
		fcntl(probe_fds[0], F_SETFD, FD_CLOEXEC);
		fcntl(probe_fds[1], F_SETFD, FD_CLOEXEC);
		// Nonblocking: if the pipe byte accounting were ever wrong, a probe
		// degrades to a spurious false instead of hanging the game thread.
		fcntl(probe_fds[0], F_SETFL, O_NONBLOCK);
		fcntl(probe_fds[1], F_SETFL, O_NONBLOCK);
	}

	static const long page_size = sysconf(_SC_PAGESIZE);
	uintptr_t first = reinterpret_cast<uintptr_t>(ptr);
	uintptr_t last = first + (len ? len - 1 : 0);
	// Address wrap (e.g. a (char*)-1 sentinel through a mismatched param):
	// the page loop below would run zero times and report "writable".
	if (last < first)
		return false;

	for (uintptr_t page = first & ~(page_size - 1); page <= (last & ~(page_size - 1)); page += page_size)
	{
		char *probe = reinterpret_cast<char*>(page < first ? first : page);
		ssize_t rc;
		do { rc = write(probe_fds[1], probe, 1); } while (rc == -1 && errno == EINTR);
		if (rc != 1)
			return false; // not readable (EFAULT) — certainly not safe to write
		do { rc = read(probe_fds[0], probe, 1); } while (rc == -1 && errno == EINTR);
		if (rc != 1)
		{
			// not writable — drain the byte we parked in the pipe
			char scratch;
			do { rc = read(probe_fds[0], &scratch, 1); } while (rc == -1 && errno == EINTR);
			return false;
		}
	}
#endif
	return true;
}

// KTP: Bounded FP_STRINGEX write-back. Replaces amx_GetStringOld, which
// copies until a zero cell with no output bound — a plugin filling the
// buffer without a terminator would write past the caller's buffer. Copies
// at most maxCells-1 chars + NUL, and only after a writability probe of the
// exact byte range.
static void amxx_stringex_writeback(char *dest, const cell *src, int maxCells,
                                    const char *fwdName, int paramIdx)
{
	if (!dest)
		return;

	int rlen = 0;
	while (rlen < maxCells - 1 && src[rlen] != 0)
		rlen++;

	if (!amxx_is_range_writable(dest, static_cast<size_t>(rlen) + 1))
	{
		AMXXLOG_Log("[AMXX] WARNING: Skipping unwritable string write-back %p in forward \"%s\" param %d (likely param type mismatch or use-after-free)",
			(void*)dest, fwdName, paramIdx);
		return;
	}

	for (int s = 0; s < rlen; s++)
		dest[s] = static_cast<char>(src[s] & 0xFF);
	dest[rlen] = '\0';
}

CForward::CForward(const char *name, ForwardExecType et, int numParams, const ForwardParam *paramTypes)
{
	m_FuncName = name;
	m_ExecType = et;
	m_NumParams = numParams;
	
	memcpy((void *)m_ParamTypes, paramTypes, numParams * sizeof(ForwardParam));
	
	// find funcs
	int func;
	m_Funcs.clear();
	
	for (CPluginMngr::iterator iter = g_plugins.begin(); iter; ++iter)
	{
		if ((*iter).isValid() && amx_FindPublic((*iter).getAMX(), name, &func) == AMX_ERR_NONE)
		{
			AMXForward tmp;
			tmp.pPlugin = &(*iter);
			tmp.func = func;
			m_Funcs.append(tmp);
		}
	}

	m_Name = name;
}

cell CForward::execute(cell *params, ForwardPreparedArray *preparedArrays)
{
	cell realParams[FORWARD_MAX_PARAMS];
	cell *physAddrs[FORWARD_MAX_PARAMS];

	const int STRINGEX_MAXLENGTH = 128;

	cell globRetVal = 0;

	for (size_t i = 0; i < m_Funcs.length(); ++i)
	{
		auto iter = &m_Funcs[i];

		if (iter->pPlugin->isExecutable(iter->func))
		{
			// Get debug info
			AMX *amx = iter->pPlugin->getAMX();
			Debugger *pDebugger = (Debugger *)amx->userdata[UD_DEBUGGER];

			if (pDebugger)
				pDebugger->BeginExec();

			// handle strings & arrays & values by reference
			int i;
			
			for (i = 0; i < m_NumParams; ++i)
			{
				if (m_ParamTypes[i] == FP_STRING || m_ParamTypes[i] == FP_STRINGEX)
				{
					const char *str = reinterpret_cast<const char*>(params[i]);
					cell *tmp;
					// KTP: bounded page-probed scan (see amxx_safe_string_length).
					int strLen = amxx_safe_string_length(str);
					if (strLen < 0)
					{
						if (str)
						{
							AMXXLOG_Log("[AMXX] WARNING: Invalid string pointer %p in global forward \"%s\" param %d (likely param type mismatch or use-after-free)",
								(void*)str, m_FuncName, i);
						}
						str = "";
						strLen = 0;
					}
					// STRINGEX allot is fixed-size; clamp so the copy can't overrun the AMX heap block
					if (m_ParamTypes[i] == FP_STRINGEX && strLen > STRINGEX_MAXLENGTH - 1)
						strLen = STRINGEX_MAXLENGTH - 1;
					amx_Allot(amx, (m_ParamTypes[i] == FP_STRING) ? strLen + 1 : STRINGEX_MAXLENGTH, &realParams[i], &tmp);
					// Inline unpacked char-to-cell copy (avoids second strlen in amx_SetStringOld)
					for (int s = 0; s < strLen; s++)
						tmp[s] = (unsigned char)str[s];
					tmp[strLen] = 0;
					physAddrs[i] = tmp;
				}
				else if (m_ParamTypes[i] == FP_ARRAY)
				{
					cell *tmp;
					amx_Allot(amx, preparedArrays[params[i]].size, &realParams[i], &tmp);
					physAddrs[i] = tmp;
					
					if (preparedArrays[params[i]].type == Type_Cell)
					{
						memcpy(tmp, preparedArrays[params[i]].ptr, preparedArrays[params[i]].size * sizeof(cell));
					} else {
						char *data = (char*)preparedArrays[params[i]].ptr;
						
						for (unsigned int j = 0; j < preparedArrays[params[i]].size; ++j)
							*tmp++ = (static_cast<cell>(*data++)) & 0xFF;
					}
				}
				else if (m_ParamTypes[i] == FP_CELL_BYREF || m_ParamTypes[i] == FP_FLOAT_BYREF)
				{
					cell *tmp;
					amx_Allot(amx, 1, &realParams[i], &tmp);
					physAddrs[i] = tmp;

					if (m_ParamTypes[i] == FP_CELL_BYREF)
					{
						memcpy(tmp, reinterpret_cast<cell *>(params[i]), sizeof(cell));
					}
					else
					{
						memcpy(tmp, reinterpret_cast<REAL *>(params[i]), sizeof(REAL));
					}
				}
				else
				{
					realParams[i] = params[i];
				}
			}
			
			//Push the parameters in reverse order. Weird, unfriendly part of Small 3.0!
			for (i = m_NumParams-1; i >= 0; i--)
			{
				amx_Push(amx, realParams[i]);
			}

			// exec
			cell retVal = 0;
#if defined BINLOG_ENABLED
			g_BinLog.WriteOp(BinLog_CallPubFunc, iter->pPlugin->getId(), iter->func);
#endif

			int err = amx_ExecPerf(amx, &retVal, iter->func);

			// log runtime error, if any
			if (err != AMX_ERR_NONE)
			{
				//Did something else set an error?
				if (pDebugger && pDebugger->ErrorExists())
				{
					//we don't care, something else logged the error.
				}
				else if (err != -1)
				{
					//nothing logged the error so spit it out anyway
					LogError(amx, err, NULL);
				}
			}

			amx->error = AMX_ERR_NONE;

			if (pDebugger)
				pDebugger->EndExec();

			// cleanup strings & arrays & values by reference
			for (i = 0; i < m_NumParams; ++i)
			{
				if (m_ParamTypes[i] == FP_STRING)
				{
					amx_Release(amx, realParams[i]);
				}
				else if (m_ParamTypes[i] == FP_STRINGEX)
				{
					// KTP: bounded write-back behind a writability probe (see amxx_stringex_writeback)
					amxx_stringex_writeback(reinterpret_cast<char*>(params[i]), physAddrs[i],
						STRINGEX_MAXLENGTH, m_FuncName, i);
					amx_Release(amx, realParams[i]);
				}
				else if (m_ParamTypes[i] == FP_ARRAY)
				{
					// copy back
					if (preparedArrays[params[i]].copyBack)
					{
						cell *tmp = physAddrs[i];
						if (preparedArrays[params[i]].type == Type_Cell)
						{
							memcpy(preparedArrays[params[i]].ptr, tmp, preparedArrays[params[i]].size * sizeof(cell));
						} else {
							char *data = (char*)preparedArrays[params[i]].ptr;
							
							for (unsigned int j = 0; j < preparedArrays[params[i]].size; ++j)
								*data++ = static_cast<char>(*tmp++ & 0xFF);
						}
					}
					amx_Release(amx, realParams[i]);
				}
				else if (m_ParamTypes[i] == FP_CELL_BYREF || m_ParamTypes[i] == FP_FLOAT_BYREF)
				{
					//copy back
					cell *tmp = physAddrs[i];
					if (m_ParamTypes[i] == FP_CELL_BYREF)
					{
						memcpy(reinterpret_cast<cell *>(params[i]), tmp, sizeof(cell));
					}
					else
					{
						memcpy(reinterpret_cast<REAL *>(params[i]), tmp, sizeof(REAL));
					}
					amx_Release(amx, realParams[i]);
				}
			}

			// decide what to do (based on exectype and retval)
			switch (m_ExecType)
			{
				case ET_IGNORE:
					break;
				case ET_STOP:
					if (retVal > 0)
						return retVal;
				case ET_STOP2:
					if (retVal == 1)
						return 1;
					else if (retVal > globRetVal)
						globRetVal = retVal;
					break;
				case ET_CONTINUE:
					if (retVal > globRetVal)
						globRetVal = retVal;
					break;
			}
		}
	}
	
	return globRetVal;
}

void CSPForward::Set(int func, AMX *amx, int numParams, const ForwardParam *paramTypes)
{
	char name[sNAMEMAX];
	m_Func = func;
	m_Amx = amx;
	m_NumParams = numParams;
	memcpy((void *)m_ParamTypes, paramTypes, numParams * sizeof(ForwardParam));
	m_HasFunc = true;
	isFree = false;
	name[0] = '\0';
	amx_GetPublic(amx, func, name);
	m_Name = name;
	m_ToDelete = false;
	m_InExec = false;
	m_RefCount = 1;
}

void CSPForward::Set(const char *funcName, AMX *amx, int numParams, const ForwardParam *paramTypes)
{
	m_Amx = amx;
	m_NumParams = numParams;
	memcpy((void *)m_ParamTypes, paramTypes, numParams * sizeof(ForwardParam));
	m_HasFunc = (amx_FindPublic(amx, funcName, &m_Func) == AMX_ERR_NONE);
	isFree = false;
	m_Name = funcName;
	m_ToDelete = false;
	m_InExec = false;
	m_RefCount = 1;
}

cell CSPForward::execute(cell *params, ForwardPreparedArray *preparedArrays)
{
	if (isFree)
		return 0;
	
	const int STRINGEX_MAXLENGTH = 128;

	cell realParams[FORWARD_MAX_PARAMS];
	cell *physAddrs[FORWARD_MAX_PARAMS];

	if (!m_HasFunc || m_ToDelete)
		return 0;

	CPluginMngr::CPlugin *pPlugin = g_plugins.findPluginFast(m_Amx);
	if (!pPlugin || !pPlugin->isExecutable(m_Func))
		return 0;

	m_InExec = true;

	Debugger *pDebugger = (Debugger *)m_Amx->userdata[UD_DEBUGGER];
	if (pDebugger)
		pDebugger->BeginExec();

	// handle strings & arrays & values by reference
	int i;
	
	for (i = 0; i < m_NumParams; ++i)
	{
		if (m_ParamTypes[i] == FP_STRING || m_ParamTypes[i] == FP_STRINGEX)
		{
			const char *str = reinterpret_cast<const char*>(params[i]);
			cell *tmp;
			// KTP: bounded page-probed scan (see amxx_safe_string_length).
			int strLen = amxx_safe_string_length(str);
			if (strLen < 0)
			{
				if (str)
				{
					AMXXLOG_Log("[AMXX] WARNING: Invalid string pointer %p in SP forward \"%s\" param %d func %d (likely param type mismatch or use-after-free)",
						(void*)str, m_Name.chars(), i, m_Func);
				}
				str = "";
				strLen = 0;
			}
			// STRINGEX allot is fixed-size; clamp so the copy can't overrun the AMX heap block
			if (m_ParamTypes[i] == FP_STRINGEX && strLen > STRINGEX_MAXLENGTH - 1)
				strLen = STRINGEX_MAXLENGTH - 1;
			amx_Allot(m_Amx, (m_ParamTypes[i] == FP_STRING) ? strLen + 1 : STRINGEX_MAXLENGTH, &realParams[i], &tmp);
			// Inline unpacked char-to-cell copy (avoids second strlen in amx_SetStringOld)
			for (int s = 0; s < strLen; s++)
				tmp[s] = (unsigned char)str[s];
			tmp[strLen] = 0;
			physAddrs[i] = tmp;
		}
		else if (m_ParamTypes[i] == FP_ARRAY)
		{
			cell *tmp;
			amx_Allot(m_Amx, preparedArrays[params[i]].size, &realParams[i], &tmp);
			physAddrs[i] = tmp;
			
			if (preparedArrays[params[i]].type == Type_Cell)
			{
				memcpy(tmp, preparedArrays[params[i]].ptr, preparedArrays[params[i]].size * sizeof(cell));
			} else {
				char *data = (char*)preparedArrays[params[i]].ptr;
				
				for (unsigned int j = 0; j < preparedArrays[params[i]].size; ++j)
					*tmp++ = (static_cast<cell>(*data++)) & 0xFF;
			}
		}
		else if (m_ParamTypes[i] == FP_CELL_BYREF || m_ParamTypes[i] == FP_FLOAT_BYREF)
		{
			cell *tmp;
			amx_Allot(m_Amx, 1, &realParams[i], &tmp);
			physAddrs[i] = tmp;

			if (m_ParamTypes[i] == FP_CELL_BYREF)
			{
				memcpy(tmp, reinterpret_cast<cell *>(params[i]), sizeof(cell));
			}
			else
			{
				memcpy(tmp, reinterpret_cast<REAL *>(params[i]), sizeof(REAL));
			}
		}
		else
		{
			realParams[i] = params[i];
		}
	}
	
	for (i = m_NumParams - 1; i >= 0; i--)
		amx_Push(m_Amx, realParams[i]);
	
	// exec
	cell retVal = 0;
#if defined BINLOG_ENABLED
	g_BinLog.WriteOp(BinLog_CallPubFunc, pPlugin->getId(), m_Func);
#endif
	int err = amx_ExecPerf(m_Amx, &retVal, m_Func);
	if (err != AMX_ERR_NONE)
	{
		//Did something else set an error?
		if (pDebugger && pDebugger->ErrorExists())
		{
			//we don't care, something else logged the error.
		}
		else if (err != -1)
		{
			//nothing logged the error so spit it out anyway
			LogError(m_Amx, err, NULL);
		}
	}
	
	if (pDebugger)
		pDebugger->EndExec();
	
	m_Amx->error = AMX_ERR_NONE;

	// cleanup strings & arrays & values by reference
	for (i = 0; i < m_NumParams; ++i)
	{
		if (m_ParamTypes[i] == FP_STRING)
		{
			amx_Release(m_Amx, realParams[i]);
		}
		else if (m_ParamTypes[i] == FP_STRINGEX)
		{
			// KTP: bounded write-back behind a writability probe (see amxx_stringex_writeback)
			amxx_stringex_writeback(reinterpret_cast<char*>(params[i]), physAddrs[i],
				STRINGEX_MAXLENGTH, m_Name.chars(), i);
			amx_Release(m_Amx, realParams[i]);
		}
		else if (m_ParamTypes[i] == FP_ARRAY)
		{
			// copy back
			if (preparedArrays[params[i]].copyBack)
			{
				cell *tmp = physAddrs[i];
				if (preparedArrays[params[i]].type == Type_Cell)
				{
					memcpy(preparedArrays[params[i]].ptr, tmp, preparedArrays[params[i]].size * sizeof(cell));
				} else {
					char *data = (char*)preparedArrays[params[i]].ptr;
					
					for (unsigned int j = 0; j < preparedArrays[params[i]].size; ++j)
						*data++ = static_cast<char>(*tmp++ & 0xFF);
				}
			}
			amx_Release(m_Amx, realParams[i]);
		}
		else if (m_ParamTypes[i] == FP_CELL_BYREF || m_ParamTypes[i] == FP_FLOAT_BYREF)
		{
			//copy back
			cell *tmp = physAddrs[i];
			if (m_ParamTypes[i] == FP_CELL_BYREF)
			{
				memcpy(reinterpret_cast<cell *>(params[i]), tmp, sizeof(cell));
			}
			else
			{
				memcpy(reinterpret_cast<REAL *>(params[i]), tmp, sizeof(REAL));
			}
			amx_Release(m_Amx, realParams[i]);
		}
	}

	m_InExec = false;

	return retVal;
}

int CForwardMngr::registerForward(const char *funcName, ForwardExecType et, int numParams, const ForwardParam * paramTypes)
{
	// KTP: Dedup -- if same function name + exec type + param count + param types exists, return it.
	// In extension mode, plugin_init fires on every map change. Multi-forwards like
	// CreateMultiForward("client_connect", ...) would otherwise accumulate duplicates.
	// IMPORTANT: Must also match paramTypes -- the same forward name could theoretically be
	// registered with different param signatures; matching on name/count alone would collide.
	for (size_t i = 0; i < m_Forwards.length(); i++)
	{
		CForward *fwd = m_Forwards[i];
		if (fwd && fwd->getExecType() == et &&
			fwd->getParamsNum() == numParams &&
			!strcmp(fwd->getFuncName(), funcName) &&
			memcmp(fwd->m_ParamTypes, paramTypes, numParams * sizeof(ForwardParam)) == 0)
		{
			return (int)(i << 1);
		}
	}

	int retVal = m_Forwards.length() << 1;
	CForward *tmp = new CForward(funcName, et, numParams, paramTypes);

	if (!tmp)
	{
		return -1;				// should be invalid
	}

	m_Forwards.append(tmp);

	return retVal;
}

int CForwardMngr::registerSPForward(int func, AMX *amx, int numParams, const ForwardParam *paramTypes)
{
	// KTP: Dedup -- if same AMX + function index + param signature exists and is active, return existing handle.
	// In extension mode, plugin_init fires on every map change without clearing forwards.
	// IMPORTANT: Must also match numParams and paramTypes -- the same function can be registered
	// as different forward types (e.g., menu callback with FP_CELL vs curl callback with FP_STRING).
	// Matching only on amx+func causes param type mismatch crashes (str=0x1 segfault in execute).
	for (size_t i = 0; i < m_SPForwards.length(); i++)
	{
		CSPForward *fwd = m_SPForwards[i];
		if (fwd && !fwd->isFree && fwd->m_Amx == amx && fwd->m_Func == func
			&& fwd->m_NumParams == numParams
			&& memcmp(fwd->m_ParamTypes, paramTypes, numParams * sizeof(ForwardParam)) == 0)
		{
			// Shared handle: count the new holder, and rescue a forward whose
			// last holder let go while it was executing.
			fwd->m_ToDelete = false;
			fwd->m_RefCount++;
			return (int)((i << 1) | 1);
		}
	}

	int retVal = -1;
	CSPForward *pForward;

	if (!m_FreeSPForwards.empty())
	{
		retVal = m_FreeSPForwards.front();
		m_FreeSPForwards.pop();  // KTP: Pop BEFORE early return to avoid stale free list entry
		pForward = m_SPForwards[retVal >> 1];
		pForward->Set(func, amx, numParams, paramTypes);

		if (pForward->getFuncsNum() == 0)
		{
			pForward->isFree = true;  // KTP: Mark free again since registration failed
			pForward->m_RefCount = 0;
			m_FreeSPForwards.push(retVal);
			return -1;
		}
	} else {
		retVal = (m_SPForwards.length() << 1) | 1;
		pForward = new CSPForward();

		if (!pForward)
			return -1;

		pForward->Set(func, amx, numParams, paramTypes);

		if (pForward->getFuncsNum() == 0)
		{
			delete pForward;
			return -1;
		}

		m_SPForwards.append(pForward);
	}

	return retVal;
}

int CForwardMngr::registerSPForward(const char *funcName, AMX *amx, int numParams, const ForwardParam *paramTypes)
{
	// KTP: Dedup -- if same AMX + function name + param signature exists and is active, return existing handle.
	// In extension mode, plugin_init fires on every map change without clearing forwards.
	// IMPORTANT: Must also match numParams and paramTypes -- the same function can be registered
	// as different forward types (e.g., menu callback with FP_CELL vs curl callback with FP_STRING).
	// Matching only on amx+name causes param type mismatch crashes (str=0x1 segfault in execute).
	for (size_t i = 0; i < m_SPForwards.length(); i++)
	{
		CSPForward *fwd = m_SPForwards[i];
		if (fwd && !fwd->isFree && fwd->m_Amx == amx
			&& !strcmp(fwd->getFuncName(), funcName)
			&& fwd->m_NumParams == numParams
			&& memcmp(fwd->m_ParamTypes, paramTypes, numParams * sizeof(ForwardParam)) == 0)
		{
			// Shared handle: count the new holder, and rescue a forward whose
			// last holder let go while it was executing.
			fwd->m_ToDelete = false;
			fwd->m_RefCount++;
			return (int)((i << 1) | 1);
		}
	}

	int retVal = (m_SPForwards.length() << 1) | 1;
	CSPForward *pForward;

	if (!m_FreeSPForwards.empty())
	{
		retVal = m_FreeSPForwards.front();
		m_FreeSPForwards.pop();  // KTP: Pop BEFORE early return to avoid stale free list entry
		pForward = m_SPForwards[retVal>>1];			// >>1 because unregisterSPForward pushes the id which contains the sp flag
		pForward->Set(funcName, amx, numParams, paramTypes);

		if (pForward->getFuncsNum() == 0)
		{
			pForward->isFree = true;  // KTP: Mark free again since registration failed
			pForward->m_RefCount = 0;
			m_FreeSPForwards.push(retVal);
			return -1;
		}
	} else {
		pForward = new CSPForward();

		if (!pForward)
			return -1;

		pForward->Set(funcName, amx, numParams, paramTypes);

		if (pForward->getFuncsNum() == 0)
		{
			delete pForward;
			return -1;
		}

		m_SPForwards.append(pForward);
	}

	return retVal;
}

bool CForwardMngr::isIdValid(int id) const
{
	return (id >= 0) && ((id & 1) ? (static_cast<size_t>(id >> 1) < m_SPForwards.length()) : (static_cast<size_t>(id >> 1) < m_Forwards.length()));
}

cell CForwardMngr::executeForwards(int id, cell *params)
{
	int retVal;
	if (id & 1)
	{
		CSPForward *fwd = m_SPForwards[id >> 1];
		retVal = fwd->execute(params, m_TmpArrays);
		if (fwd->m_ToDelete)
		{
			fwd->m_ToDelete = false;
			unregisterSPForward(id);
		}
	} else {
		retVal = m_Forwards[id >> 1]->execute(params, m_TmpArrays);
	}

	m_TmpArraysNum = 0;
	
	return retVal;
}

const char *CForwardMngr::getFuncName(int id) const
{
	if (!isIdValid(id))
	{
		return "";
	}
	return (id & 1) ? m_SPForwards[id >> 1]->getFuncName() : m_Forwards[id >> 1]->getFuncName();
}

int CForwardMngr::getFuncsNum(int id) const
{
	if (!isIdValid(id))
	{
		return 0;
	}
	return (id & 1) ? m_SPForwards[id >> 1]->getFuncsNum() : m_Forwards[id >> 1]->getFuncsNum();
}

int CForwardMngr::getParamsNum(int id) const
{
	return (id & 1) ? m_SPForwards[id >> 1]->getParamsNum() : m_Forwards[id >> 1]->getParamsNum();
}

ForwardParam CForwardMngr::getParamType(int id, int paramNum) const
{
	if (!isIdValid(id))
	{
		return FP_DONE;
	}
	return (id & 1) ? m_SPForwards[id >> 1]->getParamType(paramNum) : m_Forwards[id >> 1]->getParamType(paramNum);
}

void CForwardMngr::clear()
{
	size_t i;

	for (i = 0; i < m_Forwards.length(); ++i)
	{
		delete m_Forwards[i];
	}

	for (i = 0; i < m_SPForwards.length(); ++i)
	{
		delete m_SPForwards[i];
	}

	m_Forwards.clear();
	m_SPForwards.clear();
	
	while (!m_FreeSPForwards.empty())
	{
		m_FreeSPForwards.pop();
	}

	m_TmpArraysNum = 0;
}

bool CForwardMngr::isSPForward(int id) const
{
	return ((id & 1) == 0) ? false : true;
}

void CForwardMngr::unregisterSPForward(int id)
{
	//make sure the id is valid
	if (!isIdValid(id) || m_SPForwards.at(id >> 1)->isFree)
	{
		return;
	}

	CSPForward *fwd = m_SPForwards.at(id >> 1);

	// Dedup'd registrations share one handle. Freeing it while other holders
	// remain lets the free list recycle the id onto a different function —
	// live holders (set_task timers, menu callbacks) then execute the wrong
	// callback. Only the last release frees the slot.
	if (fwd->m_RefCount > 1)
	{
		fwd->m_RefCount--;
		return;
	}
	// Invariant tripwire: refcount 0 on a live, non-executing forward means a
	// holder released a handle it didn't own (the caller-bug class refcounting
	// can't defend against) — log it before it becomes a wrong-callback hunt.
	if (fwd->m_RefCount == 0 && !fwd->m_InExec)
	{
		AMXXLOG_Log("[AMXX] Warning: SP forward %d released with refcount already 0 (double release?)", id);
	}
	fwd->m_RefCount = 0;

	if (fwd->m_InExec)
	{
		fwd->m_ToDelete = true;
	} else {
		fwd->isFree = true;
		m_FreeSPForwards.push(id);
	}
}

int CForwardMngr::duplicateSPForward(int id)
{
	if (!isIdValid(id) || m_SPForwards.at(id >> 1)->isFree)
	{
		return -1;
	}

	CSPForward *fwd = m_SPForwards.at(id >> 1);
	
	return registerSPForward(fwd->m_Func, fwd->m_Amx, fwd->m_NumParams, fwd->m_ParamTypes);
}

int CForwardMngr::isSameSPForward(int id1, int id2)
{
	if (!isIdValid(id1) || !isIdValid(id2))
	{
		return false;
	}

	CSPForward *fwd1 = m_SPForwards.at(id1 >> 1);
	CSPForward *fwd2 = m_SPForwards.at(id2 >> 1);

	if (fwd1->isFree || fwd2->isFree)
	{
		return false;
	}

	return ((fwd1->m_Amx == fwd2->m_Amx)
			&& (fwd1->m_Func == fwd2->m_Func)
			&& (fwd1->m_NumParams == fwd2->m_NumParams));
}

int registerForwardC(const char *funcName, ForwardExecType et, cell *list, size_t num)
{
	ForwardParam params[FORWARD_MAX_PARAMS];
	
	for (size_t i=0; i<num; i++)
	{
		params[i] = static_cast<ForwardParam>(list[i]);
	}
	
	return g_forwards.registerForward(funcName, et, num, params);
}

int registerForward(const char *funcName, ForwardExecType et, ...)
{
	int curParam = 0;
	
	va_list argptr;
	va_start(argptr, et);
	
	ForwardParam params[FORWARD_MAX_PARAMS];
	ForwardParam tmp;
	
	while (true)
	{
		if (curParam == FORWARD_MAX_PARAMS)
			break;
		
		tmp = (ForwardParam)va_arg(argptr, int);
		
		if (tmp == FP_DONE)
			break;
		
		params[curParam] = tmp;
		++curParam;
	}
	
	va_end(argptr);
	
	return g_forwards.registerForward(funcName, et, curParam, params);
}

int registerSPForwardByNameC(AMX *amx, const char *funcName, cell *list, size_t num)
{
	ForwardParam params[FORWARD_MAX_PARAMS];
	
	for (size_t i=0; i<num; i++)
		params[i] = static_cast<ForwardParam>(list[i]);

	return g_forwards.registerSPForward(funcName, amx, num, params);
}

int registerSPForwardByName(AMX *amx, const char *funcName, ...)
{
	int curParam = 0;
	
	va_list argptr;
	va_start(argptr, funcName);
	
	ForwardParam params[FORWARD_MAX_PARAMS];
	ForwardParam tmp;
	
	while (true)
	{
		if (curParam == FORWARD_MAX_PARAMS)
			break;
		
		tmp = (ForwardParam)va_arg(argptr, int);
		
		if (tmp == FP_DONE)
			break;
		
		params[curParam] = tmp;
		++curParam;
	}
	
	va_end(argptr);
	
	return g_forwards.registerSPForward(funcName, amx, curParam, params);
}

int registerSPForward(AMX *amx, int func, ...)
{
	int curParam = 0;
	
	va_list argptr;
	va_start(argptr, func);
	
	ForwardParam params[FORWARD_MAX_PARAMS];
	ForwardParam tmp;
	
	while (true)
	{
		if (curParam == FORWARD_MAX_PARAMS)
			break;
		
		tmp = (ForwardParam)va_arg(argptr, int);
		
		if (tmp == FP_DONE)
			break;
		
		params[curParam] = tmp;
		++curParam;
	}
	
	va_end(argptr);
	
	return g_forwards.registerSPForward(func, amx, curParam, params);
}

cell executeForwards(int id, ...)
{
	if (!g_forwards.isIdValid(id))
		return -1;

	cell params[FORWARD_MAX_PARAMS];
	
	int paramsNum = g_forwards.getParamsNum(id);
	
	va_list argptr;
	va_start(argptr, id);

	ForwardParam param_type;
	
	for (int i = 0; i < paramsNum && i < FORWARD_MAX_PARAMS; ++i)
	{
		param_type = g_forwards.getParamType(id, i);
		if (param_type == FP_FLOAT)
		{
			REAL tmp = (REAL)va_arg(argptr, double);			// floats get converted to doubles
			params[i] = amx_ftoc(tmp);
		}
		else if(param_type == FP_FLOAT_BYREF)
		{
			REAL *tmp = reinterpret_cast<REAL *>(va_arg(argptr, double*));
			params[i] = reinterpret_cast<cell>(tmp);
		}
		else if(param_type == FP_CELL_BYREF)
		{
			cell *tmp = reinterpret_cast<cell *>(va_arg(argptr, cell*));
			params[i] = reinterpret_cast<cell>(tmp);
		}
		else
			params[i] = (cell)va_arg(argptr, cell);
	}
	
	va_end(argptr);
	
	return g_forwards.executeForwards(id, params);
}

cell CForwardMngr::prepareArray(void *ptr, unsigned int size, ForwardArrayElemType type, bool copyBack)
{
	if (m_TmpArraysNum >= FORWARD_MAX_PARAMS)
	{
#ifdef MEMORY_TEST
		m_validateAllAllocUnits();
#endif // MEMORY_TEST
		
		AMXXLOG_Log("[AMXX] Forwards with more than 32 parameters are not supported (tried to prepare array # %d).", m_TmpArraysNum + 1);
		m_TmpArraysNum = 0;
		
		return -1;
	}
	
	m_TmpArrays[m_TmpArraysNum].ptr = ptr;
	m_TmpArrays[m_TmpArraysNum].size = size;
	m_TmpArrays[m_TmpArraysNum].type = type;
	m_TmpArrays[m_TmpArraysNum].copyBack = copyBack;

	return m_TmpArraysNum++;
}

cell prepareCellArray(cell *ptr, unsigned int size, bool copyBack)
{
	return g_forwards.prepareArray((void*)ptr, size, Type_Cell, copyBack);
}

cell prepareCharArray(char *ptr, unsigned int size, bool copyBack)
{
	return g_forwards.prepareArray((void*)ptr, size, Type_Char, copyBack);
}

void unregisterSPForward(int id)
{
	g_forwards.unregisterSPForward(id);
}
