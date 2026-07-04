// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

// amxx_logging localinfo:
//  0 = no logging
//  1 = one logfile / day
//  2 = one logfile / map
//  3 = HL Logs

#include <time.h>
#if defined(_WIN32)
	#include <io.h>
	#include <windows.h>
#else
	#include <pthread.h>
#endif
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "amxmodx.h"

#if defined(_WIN32WIN32)
	#define	vsnprintf	_vsnprintf
#endif

#include <amxmodx_version.h>

// KTP: async log writer. Log()/LogError() did fopen("a+")+fprintf+fclose per
// line on the game thread; each cycle joins an ext4 journal transaction and
// can stall the frame ~165ms while a consumer-SSD journal commit is in flight.
// A dedicated thread owns the FILE*; every queued line carries its resolved
// path, so daily rollover, per-map files, and error logs share one ordered
// queue. A full queue drops the line and counts it — the game thread never
// blocks. Gate: localinfo amxx_log_async (default 1), latched in MapChange().
// pthread/CreateThread rather than std::thread: built with -fno-exceptions, so
// a failed std::thread constructor would terminate; these report by return
// value and the caller falls back to the synchronous path.

struct ktp_alogop_s
{
	char path[PLATFORM_MAX_PATH];
	char text[4096];                   // LogError queues up to 3 lines at once
};

#define KTP_ALOGQ_SIZE 1024
#define KTP_ALOGQ_MASK (KTP_ALOGQ_SIZE - 1)

static ktp_alogop_s s_ktpALogQ[KTP_ALOGQ_SIZE];
static int s_ktpALogQHead;             // game thread fills
static int s_ktpALogQTail;             // writer thread drains
static bool s_ktpALogThreadRunning;
static bool s_ktpALogStop;
static std::atomic<unsigned int> s_ktpALogDrops(0);

// Leaked on purpose, never destroyed. The join happens in ~CLog during
// dlclose, and g_log lives in another TU — destruction order across TUs is
// unspecified, so these must outlive every static destructor that can touch
// them. (dod_i386.so owns the pfnGameShutdown slot and the extension loader's
// merge is only-if-empty, so the destructor IS the production shutdown path.)
static std::mutex &KTP_ALogMx()
{
	static std::mutex *mx = new std::mutex;
	return *mx;
}
static std::condition_variable &KTP_ALogCv()
{
	static std::condition_variable *cv = new std::condition_variable;
	return *cv;
}
#if defined(_WIN32)
	static HANDLE s_ktpALogThreadHandle;
#else
	static pthread_t s_ktpALogThreadId;
#endif

static void KTP_ALogWriterLoop()
{
	FILE *fp = nullptr;                // writer-owned; game thread never sees it
	char curPath[PLATFORM_MAX_PATH] = "";
	bool needFlush = false;
	std::unique_lock<std::mutex> lk(KTP_ALogMx());
	for (;;)
	{
		while (s_ktpALogQHead == s_ktpALogQTail && !s_ktpALogStop)
		{
			if (needFlush && fp)
			{
				// Flush only once the queue drains, and off-lock — a stalled
				// flush must never block the game thread's enqueue.
				lk.unlock();
				fflush(fp);
				lk.lock();
				needFlush = false;
				continue;
			}
			KTP_ALogCv().wait(lk);
		}
		if (s_ktpALogQHead == s_ktpALogQTail && s_ktpALogStop)
			break;

		ktp_alogop_s op = s_ktpALogQ[s_ktpALogQTail];
		s_ktpALogQTail = (s_ktpALogQTail + 1) & KTP_ALOGQ_MASK;
		lk.unlock();

		if (strcmp(curPath, op.path) != 0)
		{
			if (fp)
				fclose(fp);
			fp = fopen(op.path, "a");
			if (fp)
			{
				// Line-buffered: one write() per line reaches the kernel, so a
				// crash loses at most the in-flight line — same guarantee as
				// the old synchronous open/write/close cycle.
				setvbuf(fp, nullptr, _IOLBF, 0);
				ke::SafeSprintf(curPath, sizeof(curPath), "%s", op.path);
			}
			else
				curPath[0] = '\0';     // retry the open on the next line
			needFlush = false;
		}
		if (fp)
		{
			if (fputs(op.text, fp) < 0)
			{
				// Disk full / EIO: drop the handle so the next line retries a
				// fresh open instead of silently no-oping forever.
				fclose(fp);
				fp = nullptr;
				curPath[0] = '\0';
				s_ktpALogDrops.fetch_add(1, std::memory_order_relaxed);
			}
			else
				needFlush = true;
		}
		else
			s_ktpALogDrops.fetch_add(1, std::memory_order_relaxed);

		lk.lock();
	}
	if (fp)
		fclose(fp);
}

#if defined(_WIN32)
static DWORD WINAPI KTP_ALogWriterThreadMain(LPVOID)
{
	KTP_ALogWriterLoop();
	return 0;
}
#else
static void *KTP_ALogWriterThreadMain(void *)
{
	KTP_ALogWriterLoop();
	return nullptr;
}
#endif

// Returns false if the thread can't be created — caller must fall back to the
// synchronous path for that line, not crash.
static bool KTP_ALogEnsureThread()
{
	if (s_ktpALogThreadRunning)
		return true;
#if defined(_WIN32)
	s_ktpALogThreadHandle = CreateThread(0, 0, KTP_ALogWriterThreadMain, 0, 0, nullptr);
	if (!s_ktpALogThreadHandle)
#else
	if (pthread_create(&s_ktpALogThreadId, nullptr, KTP_ALogWriterThreadMain, nullptr) != 0)
#endif
	{
		print_srvconsole("[AMXX] async log writer thread creation failed, falling back to synchronous logging\n");
		return false;
	}
	s_ktpALogThreadRunning = true;
	return true;
}

// Returns false only if the writer thread is unavailable (caller goes sync).
// A full queue counts a drop and returns true — blocking here would defeat
// the whole point.
static bool KTP_ALogEnqueue(const char *path, const char *text)
{
	if (!KTP_ALogEnsureThread())
		return false;
	{
		std::lock_guard<std::mutex> lk(KTP_ALogMx());
		int next = (s_ktpALogQHead + 1) & KTP_ALOGQ_MASK;
		if (next == s_ktpALogQTail)
		{
			s_ktpALogDrops.fetch_add(1, std::memory_order_relaxed);
			return true;
		}
		ktp_alogop_s *op = &s_ktpALogQ[s_ktpALogQHead];
		ke::SafeSprintf(op->path, sizeof(op->path), "%s", path);
		ke::SafeSprintf(op->text, sizeof(op->text), "%s", text);
		s_ktpALogQHead = next;
	}
	KTP_ALogCv().notify_one();
	return true;
}

CLog::CLog()
{
	m_LogType = 0;
	m_LogFile = nullptr;
	m_FoundError = false;
	m_LoggedErrMap = false;
	m_Async = true;
}

CLog::~CLog()
{
	CloseFile();
	AsyncShutdown();
}

// Drains the queue and joins the writer, so the last lines hit disk before
// the library unloads. Idempotent; called from Meta_Detach and the destructor.
void CLog::AsyncShutdown()
{
	if (!s_ktpALogThreadRunning)
		return;
	{
		std::lock_guard<std::mutex> lk(KTP_ALogMx());
		s_ktpALogStop = true;
	}
	KTP_ALogCv().notify_one();
#if defined(_WIN32)
	WaitForSingleObject(s_ktpALogThreadHandle, INFINITE);
	CloseHandle(s_ktpALogThreadHandle);
	s_ktpALogThreadHandle = nullptr;
#else
	pthread_join(s_ktpALogThreadId, nullptr);
#endif
	s_ktpALogThreadRunning = false;
	s_ktpALogStop = false;
}

void CLog::CloseFile()
{
	// log "log file closed" to old file, if any
	if (m_LogFile.length())
	{
		// get time
		time_t td;
		time(&td);
		tm *curTime = localtime(&td);

		char date[32];
		strftime(date, 31, "%m/%d/%Y - %H:%M:%S", curTime);

		char line[64];
		ke::SafeSprintf(line, sizeof(line), "L %s: Log file closed.\n", date);

		if (!m_Async || !KTP_ALogEnqueue(m_LogFile.chars(), line))
		{
			FILE *fp = fopen(m_LogFile.chars(), "r");

			if (fp)
			{
				fclose(fp);
				fp = fopen(m_LogFile.chars(), "a+");
				if (fp)
				{
					fputs(line, fp);
					fclose(fp);
				}
			}
		}

		m_LogFile = nullptr;
	}
}

void CLog::CreateNewFile()
{
	CloseFile();

	// build filename
	time_t td;
	time(&td);
	tm *curTime = localtime(&td);

	char file[PLATFORM_MAX_PATH];
	char name[256];
	int i = 0;

	while (true)
	{
		ke::SafeSprintf(name, sizeof(name), "%s/L%02d%02d%03d.log", g_log_dir.chars(), curTime->tm_mon + 1, curTime->tm_mday, i);
		build_pathname_r(file, sizeof(file), "%s", name);
		FILE *pTmpFile = fopen(file, "r");			// open for reading to check whether the file exists

		if (!pTmpFile)
			break;

		fclose(pTmpFile);
		++i;
	}
	m_LogFile = file;

	// Log logfile start
	char header[512];
	ke::SafeSprintf(header, sizeof(header), "AMX Mod X log file started (file \"%s\") (version \"%s\")\n", name, AMXX_VERSION);

	// Create/truncate synchronously even in async mode: the filename scan above
	// probes on-disk existence, so the file must exist before the next
	// CreateNewFile() call — a writer-thread-deferred create would let
	// back-to-back map changes reuse (and clobber) the same name.
	FILE *fp = fopen(m_LogFile.chars(), "w");

	if (!fp)
	{
		ALERT(at_logged, "[AMXX] Unexpected fatal logging error. AMXX Logging disabled.\n");
		SET_LOCALINFO("amxx_logging", "0");
		return;
	}

	if (m_Async)
	{
		fclose(fp);
		if (KTP_ALogEnqueue(m_LogFile.chars(), header))
			return;
		fp = fopen(m_LogFile.chars(), "a");
		if (!fp)
		{
			// File existed an instant ago — only fd exhaustion or an external
			// delete lands here. Header is lost; later appends still work.
			ALERT(at_logged, "[AMXX] Couldn't reopen %s for the log header.\n", m_LogFile.chars());
			return;
		}
	}

	fputs(header, fp);
	fclose(fp);
}

void CLog::UseFile(const ke::AString &fileName)
{
	static char file[PLATFORM_MAX_PATH];
	m_LogFile = build_pathname_r(file, sizeof(file), "%s/%s", g_log_dir.chars(), fileName.chars());
}

void CLog::SetLogType(const char* localInfo)
{
	m_LogType = atoi(get_localinfo(localInfo, "1"));

	if (m_LogType < 0 || m_LogType > 3)
	{
		SET_LOCALINFO(localInfo, "1");
		m_LogType = 1;

		print_srvconsole("[AMXX] Invalid amxx_logging value; setting back to 1...");
	}
}

void CLog::MapChange()
{
	// create dir if not existing
	char file[PLATFORM_MAX_PATH];
#if defined(__linux__) || defined(__APPLE__)
	mkdir(build_pathname_r(file, sizeof(file), "%s", g_log_dir.chars()), 0700);
#else
	mkdir(build_pathname_r(file, sizeof(file), "%s", g_log_dir.chars()));
#endif

	SetLogType("amxx_logging");

	// Latch async mode per map, like the engine's ktp_log_async at Log_Open.
	m_Async = atoi(get_localinfo("amxx_log_async", "1")) != 0;

	// Operator visibility for silently dropped lines (queue full / open or
	// write failure on the writer thread) — mirrors the engine's logq_drops=.
	unsigned int drops = s_ktpALogDrops.exchange(0, std::memory_order_relaxed);
	if (drops)
		print_srvconsole("[AMXX] async log writer dropped %u line(s) since last map change\n", drops);

	m_LoggedErrMap = false;

	if (m_LogType == 2)
	{
		// create new logfile
		CreateNewFile();
	} else if (m_LogType == 1) {
		Log("-------- Mapchange to %s --------", STRING(gpGlobals->mapname));
	} else {
		return;
	}
}

void CLog::Log(const char *fmt, ...)
{
	static char file[PLATFORM_MAX_PATH];

	if (m_LogType == 1 || m_LogType == 2)
	{
		// get time
		time_t td;
		time(&td);
		tm *curTime = localtime(&td);

		char date[32];
		strftime(date, 31, "%m/%d/%Y - %H:%M:%S", curTime);

		// msg
		static char msg[3072];

		va_list arglst;
		va_start(arglst, fmt);
		vsnprintf(msg, 3071, fmt, arglst);
		va_end(arglst);

		if (m_LogType == 2)
		{
			if (!m_LogFile.length())
				CreateNewFile();
			ke::SafeSprintf(file, sizeof(file), "%s", m_LogFile.chars());
		} else {
			build_pathname_r(file, sizeof(file), "%s/L%04d%02d%02d.log", g_log_dir.chars(), (curTime->tm_year + 1900), curTime->tm_mon + 1, curTime->tm_mday);
		}

		static char line[3072 + 64];
		static_assert(sizeof(line) <= sizeof(ktp_alogop_s::text), "async log queue slot smaller than Log() line buffer");
		ke::SafeSprintf(line, sizeof(line), "L %s: %s\n", date, msg);

		if (!m_Async || !KTP_ALogEnqueue(file, line))
		{
			FILE *pF = fopen(file, "a+");
			if (!pF && m_LogType == 2)
			{
				CreateNewFile();
				ke::SafeSprintf(file, sizeof(file), "%s", m_LogFile.chars());
				pF = fopen(file, "a+");
			}

			if (pF)
			{
				fputs(line, pF);
				fclose(pF);
			} else {
				ALERT(at_logged, "[AMXX] Unexpected fatal logging error (couldn't open %s for a+). AMXX Logging disabled for this map.\n", file);
				m_LogType = 0;
				return;
			}
		}

		// print on server console
		print_srvconsole("L %s: %s\n", date, msg);
	} else if (m_LogType == 3) {
		// build message
		static char msg_[3072];
		va_list arglst;
		va_start(arglst, fmt);
		vsnprintf(msg_, 3071, fmt, arglst);
		va_end(arglst);
		ALERT(at_logged, "%s\n", msg_);
	}
}

void CLog::LogError(const char *fmt, ...)
{
	static char file[PLATFORM_MAX_PATH];
	static char name[256];

	if (m_FoundError)
	{
		return;
	}

	// get time
	time_t td;
	time(&td);
	tm *curTime = localtime(&td);

	char date[32];
	strftime(date, 31, "%m/%d/%Y - %H:%M:%S", curTime);

	// msg
	static char msg[3072];

	va_list arglst;
	va_start(arglst, fmt);
	vsnprintf(msg, sizeof(msg)-1, fmt, arglst);
	va_end(arglst);

	ke::SafeSprintf(name, sizeof(name), "%s/error_%04d%02d%02d.log", g_log_dir.chars(), curTime->tm_year + 1900, curTime->tm_mon + 1, curTime->tm_mday);
	build_pathname_r(file, sizeof(file), "%s", name);

	// One op per call keeps the session header and its first error adjacent
	// even if other lines are queued between calls.
	static char text[sizeof(ktp_alogop_s::text)];
	static_assert(sizeof(text) >= sizeof(msg) + 3 * 64 + 2 * sizeof(name) + 64, "async log queue slot too small for a LogError batch");
	text[0] = '\0';
	if (!m_LoggedErrMap)
	{
		ke::SafeSprintf(text, sizeof(text), "L %s: Start of error session.\nL %s: Info (map \"%s\") (file \"%s\")\n",
			date, date, STRING(gpGlobals->mapname), name);
	}
	size_t used = strlen(text);
	ke::SafeSprintf(text + used, sizeof(text) - used, "L %s: %s\n", date, msg);

	if (m_Async && KTP_ALogEnqueue(file, text))
	{
		m_LoggedErrMap = true;
	} else {
		FILE *pF = fopen(file, "a+");

		if (pF)
		{
			fputs(text, pF);
			fclose(pF);
			m_LoggedErrMap = true;
		} else {
			ALERT(at_logged, "[AMXX] Unexpected fatal logging error (couldn't open %s for a+). AMXX Error Logging disabled for this map.\n", file);
			m_FoundError = true;
			return;
		}
	}

	// print on server console
	print_srvconsole("L %s: %s\n", date, msg);
}
