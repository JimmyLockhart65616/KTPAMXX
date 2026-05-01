/**
 * ktp_version_reporter — shared KTP plugin version registry + rcon command.
 *
 * Each KTP plugin should `#include <ktp_version_reporter>` and call
 * `KTP_RegisterVersion(PLUGIN_NAME, PLUGIN_VERSION)` from `plugin_init()`.
 *
 * --- v2 design (2026-04-30): multi-forward, no shared storage ----------
 *
 * The original (v1) design accumulated all entries into a single
 * `_ktp_v_data` localinfo string. That hits the engine's per-value cap
 * (MAX_KV_LEN = 127, see `KTPReHLDS/rehlds/engine/info.h:34`) once 3+
 * plugins register: `Info_SetValueForStarKey` silently rejects writes that
 * push the value past 127 chars, while AMXX's `set_localinfo` returns
 * success regardless. Result observed in production 2026-04-30: only the
 * first 2 plugins by plugins.ini load order ever appeared in the dump.
 *
 * v2 abandons shared storage. Each plugin owns its own one-line dump via
 * an AMXX multi-forward (`KTP_OnVersionDump`). The first plugin to call
 * `KTP_RegisterVersion` (detected via the `_ktp_v_reg` localinfo flag,
 * which only stores "1" — well under the 127-byte cap) registers the
 * `amx_ktp_versions` rcon concmd AND creates the multi-forward. When the
 * concmd fires, every plugin's own `public KTP_OnVersionDump(id, counter[1])`
 * runs in load order; each prints its own line and increments the
 * copyback counter for the totals footer.
 *
 * --- Build-time SHA + time -----------------------------------------------
 *
 * Build-time SHA + time are injected via a generated `build_info.inc` in
 * each plugin's compile.sh (see compile.sh template below). If absent,
 * "unknown" is used as a graceful fallback so the include works even
 * outside the canonical toolchain.
 *
 *   GIT_SHA=$(git -C "$SCRIPT_DIR" rev-parse --short HEAD 2>/dev/null || echo "unknown")
 *   BUILD_TIME=$(date -u +%Y-%m-%dT%H:%MZ)
 *   cat > "$TEMP_BUILD/build_info.inc" <<EOF
 *   #define KTP_BUILD_SHA "$GIT_SHA"
 *   #define KTP_BUILD_TIME "$BUILD_TIME"
 *   EOF
 *
 * Rolled out 2026-04-25 as the first concrete deliverable for the test
 * infrastructure plan; v2 fix shipped 2026-04-30 after production
 * observation revealed the localinfo cap. Cross-references:
 * `KTPInfrastructure/TEST_INFRASTRUCTURE_PLAN.md` (test infra plan),
 * `KTPReHLDS/rehlds/engine/info.h` + `info.cpp:611` (engine cap source).
 */

#if defined _ktp_version_reporter_included
    #endinput
#endif
#define _ktp_version_reporter_included

#include <amxmodx>
#include <amxmisc>  // for cmd_access — keeps the include self-contained for plugins that don't already pull amxmisc

#tryinclude "build_info"
#if !defined KTP_BUILD_SHA
    #define KTP_BUILD_SHA "unknown"
#endif
#if !defined KTP_BUILD_TIME
    #define KTP_BUILD_TIME "unknown"
#endif

// Localinfo flag — used only as a one-bit "first registrant" signal.
// Holds "1" once set; well under the engine's 127-byte per-value cap.
#define _KTP_VR_KEY_FLAG "_ktp_v_reg"

// Per-plugin name/version storage (plugin-local; no engine info-string involvement)
#define _KTP_VR_NAME_MAX 64
#define _KTP_VR_VER_MAX  32

new g_ktpVrSelfName[_KTP_VR_NAME_MAX];
new g_ktpVrSelfVer[_KTP_VR_VER_MAX];

// Multi-forward handle; only meaningful in the first registrant's address
// space. Other plugins keep -1 here, which is fine — only the first
// plugin's `_ktp_v_dump_handler` is reachable via the registered concmd.
new g_ktpVrFwdId = -1;

stock KTP_RegisterVersion(const plName[], const plVersion[]) {
    // Stash own data so this plugin's KTP_OnVersionDump callback has it.
    copy(g_ktpVrSelfName, charsmax(g_ktpVrSelfName), plName);
    copy(g_ktpVrSelfVer,  charsmax(g_ktpVrSelfVer),  plVersion);

    new flag[8];
    get_localinfo(_KTP_VR_KEY_FLAG, flag, charsmax(flag));
    if (flag[0] == 0) {
        // First registration this server run — register the rcon concmd
        // and create the multi-forward. Subsequent plugins skip this.
        set_localinfo(_KTP_VR_KEY_FLAG, "1");
        register_concmd(
            "amx_ktp_versions",
            "_ktp_v_dump_handler",
            ADMIN_RCON,
            "List loaded KTP plugin versions (name, version, git SHA, build time)"
        );
        g_ktpVrFwdId = CreateMultiForward("KTP_OnVersionDump", ET_IGNORE, FP_CELL, FP_ARRAY);
    }
}

public _ktp_v_dump_handler(id, level, cid) {
    if (!cmd_access(id, level, cid, 1)) return PLUGIN_HANDLED;

    if (g_ktpVrFwdId < 0) {
        // Defensive: this handler should only be reachable from the plugin
        // that created the forward, but bail rather than crash if not.
        console_print(id, "^nKTP version reporter: forward not initialized");
        return PLUGIN_HANDLED;
    }

    console_print(id, "^nKTP plugin versions:");
    console_print(id, "  %-32s %-14s %-10s %s",
                  "Name", "Version", "SHA", "Build Time (UTC)");
    console_print(id, "  %-32s %-14s %-10s %s",
                  "--------------------------------",
                  "--------------",
                  "----------",
                  "------------------");

    // Counter passed by FP_ARRAY with copyback=1 — each plugin increments
    // counter[0]. AMXX-fork convention since FP_CELL_BYREF is unavailable.
    new counter[1] = {0};
    new arrHandle = PrepareArray(counter, 1, 1);
    new ret;
    ExecuteForward(g_ktpVrFwdId, ret, id, arrHandle);

    console_print(id, "^n  Total: %d KTP plugin(s) loaded", counter[0]);
    return PLUGIN_HANDLED;
}

// Per-plugin version dump callback. AMXX matches by name across all loaded
// plugins when ExecuteForward fires; each plugin's own copy runs against
// its own g_ktpVrSelfName / g_ktpVrSelfVer.
//
// Guard against the edge case where a plugin includes this file but
// never calls KTP_RegisterVersion — empty SelfName means we skip the print
// and don't bump the counter, so the dump stays accurate.
public KTP_OnVersionDump(id, counter[1]) {
    if (g_ktpVrSelfName[0] == 0) return;
    console_print(id, "  %-32s %-14s %-10s %s",
                  g_ktpVrSelfName, g_ktpVrSelfVer, KTP_BUILD_SHA, KTP_BUILD_TIME);
    counter[0]++;
}
